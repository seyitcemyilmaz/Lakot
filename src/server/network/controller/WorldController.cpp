#include "WorldController.h"

#include <filesystem>
#include <iostream>
#include <syncstream>

#include "../NetworkManager.h"

using namespace lakot;

WorldController::WorldController(NetworkManager& pNetworkManager,
                                 RepositoryManager& pRepositoryManager,
                                 const ItemCatalog& pItemCatalog,
                                 const MapCatalog& pMapCatalog,
                                 const MonsterCatalog& pMonsterCatalog)
    : BaseController(pNetworkManager, pRepositoryManager)
    , mItemCatalog(pItemCatalog)
    , mMapCatalog(pMapCatalog)
    , mMonsterCatalog(pMonsterCatalog)
{

}

void WorldController::initialize()
{
    // One zone per map in the catalog. Built here rather than in the
    // constructor because a zone needs the ItemCatalog, and that is only
    // loaded once the database is up. Never modified afterwards, so mZones
    // needs no lock.
    for (uint32_t tMapId : mMapCatalog.getMapIds())
    {
        mZones.emplace(tMapId, std::make_unique<Zone>(*mMapCatalog.getMap(tMapId), mMapCatalog, mItemCatalog, mMonsterCatalog));
    }

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kPlayerStateUpdate, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePlayerStateUpdate(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kAttackRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleAttackRequest(pSession, pMessage);
        }
    );

    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kPickupItemRequest, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePickupItemRequest(pSession, pMessage);
        }
    );

    mNetworkManager.setOnUserDisconnected(
    [this](uint64_t pCharacterId)
    {
        this->handlePlayerDisconnect(pCharacterId);
    });

    for (auto& [tMapId, tZone] : mZones)
    {
        tZone->start(
            [this](uint64_t pCharacterId, Zone::Packet pPacket)
            {
                this->sendToCharacter(pCharacterId, std::move(pPacket));
            },
            [this](const CharacterSnapshot& pCharacter, uint32_t pTargetMapId, const PlayerState& pSpawnState)
            {
                this->transferCharacter(pCharacter, pTargetMapId, pSpawnState);
            },
            [this](const Zone::PersistData& pData)
            {
                mRepositoryManager.getCharacterRepository().saveCharacterState(
                    pData.characterId, pData.mapId,
                    pData.position.x, pData.position.y, pData.position.z, pData.position.yaw,
                    pData.stats);

                mRepositoryManager.getItemRepository().saveCharacterItems(
                    pData.characterId, pData.items);
            },
            // Resolved against the executable, not the source tree - the same
            // reasoning as the game client's asset path.
            (std::filesystem::path(mScriptDirectory)).string());
    }
}

void WorldController::shutdown()
{
    for (auto& [tMapId, tZone] : mZones)
    {
        tZone->stop();
    }
}

Zone* WorldController::getZone(uint32_t pMapId)
{
    auto tIterator = mZones.find(pMapId);
    return tIterator == mZones.end() ? nullptr : tIterator->second.get();
}

std::optional<uint32_t> WorldController::getCharacterZone(uint64_t pCharacterId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mCharacterZone.find(pCharacterId);

    if (tIterator == mCharacterZone.end())
    {
        return std::nullopt;
    }

    return tIterator->second;
}

void WorldController::enterWorld(const CharacterSnapshot& pCharacter,
                                 uint32_t pMapId,
                                 const PlayerState& pState)
{
    uint64_t pCharacterId = pCharacter.characterId;
    const std::string& pName = pCharacter.name;

    // AuthController already relocates characters saved on a map that no
    // longer exists; this is the last line of defence against landing in no
    // zone at all.
    Zone* tZone = getZone(pMapId);

    if (!tZone)
    {
        std::osyncstream(std::cerr) << "[World] Bilinmeyen harita id: " << pMapId << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);
        mCharacterZone[pCharacterId] = pMapId;
        mCharacterNames[pCharacterId] = pName;
        mNameToCharacterId[pName] = pCharacterId;
    }

    tZone->enterPlayer(pCharacter, pState);
}

void WorldController::handleAttackRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();
    auto tMapId = tCharacterId != 0 ? getCharacterZone(tCharacterId) : std::nullopt;
    Zone* tZone = tMapId ? getZone(*tMapId) : nullptr;

    if (tZone)
    {
        const auto& tRequest = pMessage.request().attack_request();
        tZone->attack(tCharacterId, tRequest.yaw(), tRequest.combo());
    }
}

void WorldController::handlePickupItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();
    auto tMapId = tCharacterId != 0 ? getCharacterZone(tCharacterId) : std::nullopt;
    Zone* tZone = tMapId ? getZone(*tMapId) : nullptr;

    if (tZone)
    {
        tZone->pickUp(tCharacterId, pMessage.request().pickup_item_request().entity_id(), pMessage.request().header().id());
    }
}

void WorldController::handlePlayerStateUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tCharacterId = pSession->getCharacterId();

    if (tCharacterId == 0)
    {
        return; // authenticated but not in the world yet (character select)
    }

    auto tMapId = getCharacterZone(tCharacterId);

    if (!tMapId)
    {
        return; // not in any zone (never entered the world, or already left)
    }

    Zone* tZone = getZone(*tMapId);

    if (!tZone)
    {
        return;
    }

    const auto& tUpdate = pMessage.request().player_state_update();
    const auto& tPosition = tUpdate.position();

    PlayerState tState;
    tState.x = tPosition.x();
    tState.y = tPosition.y();
    tState.z = tPosition.z();
    tState.yaw = tUpdate.yaw();

    // Posts into the zone thread and returns - no world state is read or
    // written on this I/O thread, and no lock is taken.
    tZone->submitState(tCharacterId, tState);
}

void WorldController::resumeCharacter(uint64_t pCharacterId)
{
    auto tMapId = getCharacterZone(pCharacterId);

    if (!tMapId)
    {
        return;
    }

    if (Zone* tZone = getZone(*tMapId))
    {
        tZone->resumePlayer(pCharacterId);
    }
}

void WorldController::handlePlayerDisconnect(uint64_t pCharacterId)
{
    std::optional<uint32_t> tMapId;

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        auto tIterator = mCharacterZone.find(pCharacterId);

        if (tIterator == mCharacterZone.end())
        {
            return; // never entered the world - nothing to clean up
        }

        tMapId = tIterator->second;
        mCharacterZone.erase(tIterator);

        auto tNameIterator = mCharacterNames.find(pCharacterId);

        if (tNameIterator != mCharacterNames.end())
        {
            mNameToCharacterId.erase(tNameIterator->second);
            mCharacterNames.erase(tNameIterator);
        }
    }

    // The zone persists the final position on its own thread as part of
    // leaving - see Zone::leavePlayer.
    if (Zone* tZone = getZone(*tMapId))
    {
        tZone->leavePlayer(pCharacterId);
    }
}

void WorldController::transferCharacter(const CharacterSnapshot& pCharacter,
                                        uint32_t pTargetMapId,
                                        const PlayerState& pSpawnState)
{
    uint64_t pCharacterId = pCharacter.characterId;

    Zone* tTargetZone = getZone(pTargetMapId);

    if (!tTargetZone)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        // Gone (disconnected mid-transfer) - the source zone already dropped
        // them, so there is nothing left to move.
        if (mCharacterZone.find(pCharacterId) == mCharacterZone.end())
        {
            return;
        }

        mCharacterZone[pCharacterId] = pTargetMapId;
    }

    // Sent BEFORE the character is added to the destination zone. The client
    // clears its whole remote-entity list on this message (none of the old
    // map's entities belong in the new view), and TCP preserves order - so
    // sending it after the first snapshot from the new zone would wipe out
    // the arrivals it had just been told about.
    sendMapChanged(pCharacterId, pTargetMapId, pSpawnState);

    tTargetZone->enterPlayer(pCharacter, pSpawnState);
}

void WorldController::sendToCharacter(uint64_t pCharacterId, Zone::Packet pPacket)
{
    // getByCharacter, not get(): the registry's primary index is keyed by
    // account, and a zone only ever knows character ids.
    if (auto tSession = mNetworkManager.getSessionRegistry().getByCharacter(pCharacterId))
    {
        tSession->send(std::move(pPacket));
    }
}

void WorldController::sendMapChanged(uint64_t pCharacterId, uint32_t pMapId, const PlayerState& pState)
{
    connection::Message tMessage;
    auto* tChanged = tMessage.mutable_response()->mutable_map_changed();
    tChanged->set_map_id(pMapId);
    tChanged->mutable_position()->set_x(pState.x);
    tChanged->mutable_position()->set_y(pState.y);
    tChanged->mutable_position()->set_z(pState.z);
    tChanged->set_yaw(pState.yaw);

    sendToCharacter(pCharacterId, NetworkSession<connection::Message>::makePacket(tMessage));
}

std::string WorldController::getCharacterName(uint64_t pCharacterId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mCharacterNames.find(pCharacterId);

    if (tIterator != mCharacterNames.end())
    {
        return tIterator->second;
    }

    return "Player" + std::to_string(pCharacterId); // shouldn't normally happen
}

std::optional<uint64_t> WorldController::findOnlineCharacterIdByName(const std::string& pName)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mNameToCharacterId.find(pName);

    if (tIterator == mNameToCharacterId.end())
    {
        return std::nullopt;
    }

    return tIterator->second;
}

void WorldController::withCharacter(uint64_t pCharacterId, std::function<bool(Character&)> pAction)
{
    auto tMapId = getCharacterZone(pCharacterId);

    if (!tMapId)
    {
        return;
    }

    if (Zone* tZone = getZone(*tMapId))
    {
        tZone->withCharacter(pCharacterId, std::move(pAction));
    }
}

void WorldController::sendNearby(uint64_t pOriginCharacterId, Zone::Packet pPacket)
{
    auto tMapId = getCharacterZone(pOriginCharacterId);

    if (!tMapId)
    {
        return;
    }

    if (Zone* tZone = getZone(*tMapId))
    {
        tZone->sendNearby(pOriginCharacterId, std::move(pPacket));
    }
}
