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
            [this](uint32_t pSourceMapId, const CharacterSnapshot& pCharacter, uint32_t pTargetMapId, const PlayerState& pSpawnState)
            {
                this->transferCharacter(pSourceMapId, pCharacter, pTargetMapId, pSpawnState);
            },
            [this](const Zone::PersistData& pData)
            {
                CharacterRepository::CharacterSave tSave;
                tSave.characterId = pData.characterId;
                tSave.mapId = pData.mapId;
                tSave.x = pData.position.x;
                tSave.y = pData.position.y;
                tSave.z = pData.position.z;
                tSave.yaw = pData.position.yaw;
                tSave.stats = pData.stats;
                tSave.items = pData.items;

                mRepositoryManager.getCharacterRepository().saveCharacter(std::move(tSave));
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

std::optional<CharacterAccess> WorldController::getCharacterAccess(uint64_t pCharacterId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mCharacterAccess.find(pCharacterId);

    if (tIterator == mCharacterAccess.end())
    {
        return std::nullopt;
    }

    return tIterator->second;
}

void WorldController::setMutedUntil(uint64_t pCharacterId, int64_t pEpochSeconds)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mCharacterAccess.find(pCharacterId);

    if (tIterator != mCharacterAccess.end())
    {
        tIterator->second.mutedUntilEpoch = pEpochSeconds;
    }
}

std::optional<uint64_t> WorldController::findOnlineCharacterIdByAccount(uint64_t pAccountId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    for (const auto& [tCharacterId, tAccess] : mCharacterAccess)
    {
        if (tAccess.accountId == pAccountId)
        {
            return tCharacterId;
        }
    }

    return std::nullopt;
}

size_t WorldController::getOnlineCount()
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);
    return mCharacterZone.size();
}

Zone* WorldController::findCharacterZone(uint64_t pCharacterId)
{
    auto tMapId = getCharacterZone(pCharacterId);
    return tMapId ? getZone(*tMapId) : nullptr;
}

void WorldController::enterWorld(const CharacterSnapshot& pCharacter,
                                 uint32_t pMapId,
                                 const PlayerState& pState,
                                 const CharacterAccess& pAccess)
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
        mCharacterAccess[pCharacterId] = pAccess;

        tZone->enterPlayer(pCharacter, pState);
    }
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
    Zone* tZone = nullptr;

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        auto tIterator = mCharacterZone.find(pCharacterId);

        if (tIterator == mCharacterZone.end())
        {
            return; // never entered the world - nothing to clean up
        }

        tZone = getZone(tIterator->second);
        mCharacterZone.erase(tIterator);
        mCharacterAccess.erase(pCharacterId);

        auto tNameIterator = mCharacterNames.find(pCharacterId);

        if (tNameIterator != mCharacterNames.end())
        {
            mNameToCharacterId.erase(tNameIterator->second);
            mCharacterNames.erase(tNameIterator);
        }

        if (tZone)
        {
            mReleaseWaiters.try_emplace(pCharacterId);

            tZone->leavePlayer(pCharacterId, [this, pCharacterId]()
            {
                this->onCharacterReleased(pCharacterId);
            });
        }
    }
}

void WorldController::onCharacterReleased(uint64_t pCharacterId)
{
    std::vector<std::function<void()>> tWaiters;

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        auto tIterator = mReleaseWaiters.find(pCharacterId);

        if (tIterator == mReleaseWaiters.end())
        {
            return;
        }

        tWaiters = std::move(tIterator->second);
        mReleaseWaiters.erase(tIterator);
    }

    for (auto& tWaiter : tWaiters)
    {
        tWaiter();
    }
}

void WorldController::runAfterRelease(uint64_t pCharacterId, std::function<void()> pAction)
{
    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        auto tIterator = mReleaseWaiters.find(pCharacterId);

        if (tIterator != mReleaseWaiters.end())
        {
            tIterator->second.push_back(std::move(pAction));
            return;
        }
    }

    pAction();
}

void WorldController::transferCharacter(uint32_t pSourceMapId,
                                        const CharacterSnapshot& pCharacter,
                                        uint32_t pTargetMapId,
                                        const PlayerState& pSpawnState)
{
    uint64_t pCharacterId = pCharacter.characterId;

    Zone* tTargetZone = getZone(pTargetMapId);

    if (!tTargetZone)
    {
        return;
    }

    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mCharacterZone.find(pCharacterId);

    if (tIterator == mCharacterZone.end() || tIterator->second != pSourceMapId)
    {
        std::osyncstream(std::cout) << "[World] Gecis dusuruldu (karakter artik kaynak haritada degil): " << pCharacterId << std::endl;
        return;
    }

    tIterator->second = pTargetMapId;

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

void WorldController::useItem(uint64_t pCharacterId, const ItemTemplate& pTemplate, Zone::UseItemCallback pCallback)
{
    if (!pCallback)
    {
        return;
    }

    auto tMapId = getCharacterZone(pCharacterId);
    Zone* tZone = tMapId ? getZone(*tMapId) : nullptr;

    if (!tZone)
    {
        pCallback(services::inventory::USE_ITEM_STATUS_INVALID, 0.0f);
        return;
    }

    tZone->useItem(pCharacterId, pTemplate, std::move(pCallback));
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
