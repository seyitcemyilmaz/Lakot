#include "WorldController.h"

#include "../NetworkManager.h"

using namespace lakot;

WorldController::WorldController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager)
    : BaseController(pNetworkManager, pRepositoryManager)
{
    // Fixed, known set of maps for v1 - populated once up front so
    // mRegistries never needs to be modified (and therefore never needs its
    // own lock) after construction.
    mRegistries.try_emplace(services::world::MAP_TOWN);
    mRegistries.try_emplace(services::world::MAP_FOREST);
}

void WorldController::initialize()
{
    mNetworkManager.getDispatcher().registerHandler(protocol::Request::kPlayerStateUpdate, PacketType::Request,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePlayerStateUpdate(pSession, pMessage);
        }
    );

    mNetworkManager.setOnUserDisconnected(
    [this](uint64_t pUserId)
    {
        this->handlePlayerDisconnect(pUserId);
    });
}

void WorldController::handlePlayerStateUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    uint64_t tUserId = pSession->getUserId();

    if (tUserId == 0)
    {
        return; // not logged in yet - ignore
    }

    const auto& tUpdate = pMessage.request().player_state_update();
    const auto& tPosition = tUpdate.position();

    PlayerState tState;
    tState.x = tPosition.x();
    tState.y = tPosition.y();
    tState.z = tPosition.z();
    tState.yaw = tUpdate.yaw();

    services::world::MapId tCurrentMapId = getOrAssignPlayerMap(tUserId);

    if (auto tPortal = MapCatalog::findPortalAt(tCurrentMapId, tState.x, tState.z))
    {
        teleportPlayer(tUserId, tCurrentMapId, *tPortal);
        return;
    }

    WorldRegistry& tRegistry = getRegistryForMap(tCurrentMapId);
    VisibilityChange tChange = tRegistry.updatePlayer(tUserId, tState);

    for (uint64_t tOtherId : tChange.entered)
    {
        // Tell the already-present player about the mover, and the mover
        // about the already-present player - discovery is symmetric.
        sendPlayerJoined(tOtherId, tUserId, tState);

        if (auto tOtherState = tRegistry.getState(tOtherId))
        {
            sendPlayerJoined(tUserId, tOtherId, *tOtherState);
        }
    }

    for (uint64_t tOtherId : tChange.exited)
    {
        sendPlayerLeft(tOtherId, tUserId);
        sendPlayerLeft(tUserId, tOtherId);
    }

    if (!tChange.stillNearby.empty())
    {
        connection::Message tBroadcastMessage;
        auto* tBroadcast = tBroadcastMessage.mutable_response()->mutable_player_state_broadcast();
        tBroadcast->set_player_id(tUserId);
        tBroadcast->mutable_position()->set_x(tState.x);
        tBroadcast->mutable_position()->set_y(tState.y);
        tBroadcast->mutable_position()->set_z(tState.z);
        tBroadcast->set_yaw(tState.yaw);

        for (uint64_t tOtherId : tChange.stillNearby)
        {
            if (auto tOtherSession = mNetworkManager.getSessionRegistry().get(tOtherId))
            {
                tOtherSession->send(tBroadcastMessage);
            }
        }
    }
}

void WorldController::handlePlayerDisconnect(uint64_t pUserId)
{
    services::world::MapId tMapId;

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

        auto tIterator = mPlayerMapAssignment.find(pUserId);

        if (tIterator == mPlayerMapAssignment.end())
        {
            return; // never sent a state update - nothing to clean up
        }

        tMapId = static_cast<services::world::MapId>(tIterator->second);
        mPlayerMapAssignment.erase(tIterator);

        auto tUsernameIterator = mPlayerUsernames.find(pUserId);
        if (tUsernameIterator != mPlayerUsernames.end())
        {
            mUsernameToUserId.erase(tUsernameIterator->second);
            mPlayerUsernames.erase(tUsernameIterator);
        }
    }

    WorldRegistry& tRegistry = getRegistryForMap(tMapId);

    if (auto tState = tRegistry.getState(pUserId))
    {
        mRepositoryManager.getAccountRepository().savePlayerState(
            pUserId, static_cast<uint32_t>(tMapId), tState->x, tState->y, tState->z, tState->yaw);
    }

    std::vector<uint64_t> tWatchers = tRegistry.removePlayer(pUserId);

    for (uint64_t tWatcherId : tWatchers)
    {
        sendPlayerLeft(tWatcherId, pUserId);
    }
}

void WorldController::seedPlayerMap(uint64_t pUserId, services::world::MapId pMapId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);
    mPlayerMapAssignment[pUserId] = pMapId;
}

void WorldController::setPlayerUsername(uint64_t pUserId, const std::string& pUsername)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);
    mPlayerUsernames[pUserId] = pUsername;
    mUsernameToUserId[pUsername] = pUserId;
}

std::string WorldController::getPlayerUsername(uint64_t pUserId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mPlayerUsernames.find(pUserId);

    if (tIterator != mPlayerUsernames.end())
    {
        return tIterator->second;
    }

    return "Player" + std::to_string(pUserId); // shouldn't normally happen
}

std::optional<uint64_t> WorldController::findOnlinePlayerIdByUsername(const std::string& pUsername)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mUsernameToUserId.find(pUsername);

    if (tIterator == mUsernameToUserId.end())
    {
        return std::nullopt;
    }

    return tIterator->second;
}

services::world::MapId WorldController::getOrAssignPlayerMap(uint64_t pUserId)
{
    std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);

    auto tIterator = mPlayerMapAssignment.find(pUserId);

    if (tIterator != mPlayerMapAssignment.end())
    {
        return static_cast<services::world::MapId>(tIterator->second);
    }

    mPlayerMapAssignment[pUserId] = services::world::MAP_TOWN;
    return services::world::MAP_TOWN;
}

WorldRegistry& WorldController::getRegistryForMap(services::world::MapId pMapId)
{
    return mRegistries.at(static_cast<uint32_t>(pMapId));
}

void WorldController::teleportPlayer(uint64_t pUserId, services::world::MapId pFromMapId, const PortalDefinition& pPortal)
{
    WorldRegistry& tFromRegistry = getRegistryForMap(pFromMapId);
    std::vector<uint64_t> tOldWatchers = tFromRegistry.removePlayer(pUserId);

    for (uint64_t tWatcherId : tOldWatchers)
    {
        sendPlayerLeft(tWatcherId, pUserId);
    }

    {
        std::lock_guard<std::mutex> tLock(mPlayerMetaMutex);
        mPlayerMapAssignment[pUserId] = pPortal.targetMapId;
    }

    PlayerState tSpawnState;
    tSpawnState.x = pPortal.targetX;
    tSpawnState.y = pPortal.targetY;
    tSpawnState.z = pPortal.targetZ;
    tSpawnState.yaw = pPortal.targetYaw;

    WorldRegistry& tToRegistry = getRegistryForMap(pPortal.targetMapId);
    VisibilityChange tChange = tToRegistry.updatePlayer(pUserId, tSpawnState);

    for (uint64_t tOtherId : tChange.entered)
    {
        sendPlayerJoined(tOtherId, pUserId, tSpawnState);

        if (auto tOtherState = tToRegistry.getState(tOtherId))
        {
            sendPlayerJoined(pUserId, tOtherId, *tOtherState);
        }
    }

    sendMapChanged(pUserId, pPortal.targetMapId, tSpawnState);
}

void WorldController::sendPlayerJoined(uint64_t pTargetUserId, uint64_t pPlayerId, const PlayerState& pState)
{
    auto tTargetSession = mNetworkManager.getSessionRegistry().get(pTargetUserId);

    if (!tTargetSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tJoined = tMessage.mutable_response()->mutable_player_joined();
    tJoined->set_player_id(pPlayerId);
    tJoined->set_username(getPlayerUsername(pPlayerId));
    tJoined->mutable_position()->set_x(pState.x);
    tJoined->mutable_position()->set_y(pState.y);
    tJoined->mutable_position()->set_z(pState.z);
    tJoined->set_yaw(pState.yaw);

    tTargetSession->send(tMessage);
}

void WorldController::sendPlayerLeft(uint64_t pTargetUserId, uint64_t pPlayerId)
{
    auto tTargetSession = mNetworkManager.getSessionRegistry().get(pTargetUserId);

    if (!tTargetSession)
    {
        return;
    }

    connection::Message tMessage;
    tMessage.mutable_response()->mutable_player_left()->set_player_id(pPlayerId);

    tTargetSession->send(tMessage);
}

void WorldController::sendMapChanged(uint64_t pTargetUserId, services::world::MapId pMapId, const PlayerState& pState)
{
    auto tTargetSession = mNetworkManager.getSessionRegistry().get(pTargetUserId);

    if (!tTargetSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tChanged = tMessage.mutable_response()->mutable_map_changed();
    tChanged->set_map_id(pMapId);
    tChanged->mutable_position()->set_x(pState.x);
    tChanged->mutable_position()->set_y(pState.y);
    tChanged->mutable_position()->set_z(pState.z);
    tChanged->set_yaw(pState.yaw);

    tTargetSession->send(tMessage);
}
