#include "WorldController.h"

#include "../NetworkManager.h"

using namespace lakot;

WorldController::WorldController(NetworkManager* pNetworkManager)
    : mNetworkManager(pNetworkManager)
{

}

void WorldController::initialize()
{
    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kPlayerJoined, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePlayerJoined(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kPlayerStateBroadcast, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePlayerStateBroadcast(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kPlayerLeft, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handlePlayerLeft(pSession, pMessage);
        }
    );

    mNetworkManager->getDispatcher().registerHandler(protocol::Response::kMapChanged, PacketType::Response,
        [this](std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
        {
            this->handleMapChanged(pSession, pMessage);
        }
    );
}

void WorldController::sendPlayerStateUpdate(float pX, float pY, float pZ, float pYaw)
{
    auto tSession = mNetworkManager->getClient().getSession();

    if (!tSession)
    {
        return;
    }

    connection::Message tMessage;
    auto* tRequest = tMessage.mutable_request();
    auto* tUpdate = tRequest->mutable_player_state_update();
    tUpdate->mutable_position()->set_x(pX);
    tUpdate->mutable_position()->set_y(pY);
    tUpdate->mutable_position()->set_z(pZ);
    tUpdate->set_yaw(pYaw);

    tSession->send(tMessage);
}

void WorldController::setPlayerJoinedCallback(PlayerJoinedCallback pCallback)
{
    mPlayerJoinedCallback = pCallback;
}

void WorldController::setPlayerStateUpdateCallback(PlayerStateUpdateCallback pCallback)
{
    mPlayerStateUpdateCallback = pCallback;
}

void WorldController::setPlayerLeftCallback(PlayerLeftCallback pCallback)
{
    mPlayerLeftCallback = pCallback;
}

void WorldController::setMapChangedCallback(MapChangedCallback pCallback)
{
    mMapChangedCallback = pCallback;
}

void WorldController::handlePlayerJoined(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mPlayerJoinedCallback)
    {
        return;
    }

    const auto& tJoined = pMessage.response().player_joined();
    const auto& tPosition = tJoined.position();

    PlayerSnapshot tSnapshot{ tJoined.player_id(), tPosition.x(), tPosition.y(), tPosition.z(), tJoined.yaw(), tJoined.username() };

    mPlayerJoinedCallback(tSnapshot);
}

void WorldController::handlePlayerStateBroadcast(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mPlayerStateUpdateCallback)
    {
        return;
    }

    const auto& tUpdate = pMessage.response().player_state_broadcast();
    const auto& tPosition = tUpdate.position();

    PlayerSnapshot tSnapshot{ tUpdate.player_id(), tPosition.x(), tPosition.y(), tPosition.z(), tUpdate.yaw() };

    mPlayerStateUpdateCallback(tSnapshot);
}

void WorldController::handlePlayerLeft(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mPlayerLeftCallback)
    {
        return;
    }

    mPlayerLeftCallback(pMessage.response().player_left().player_id());
}

void WorldController::handleMapChanged(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage)
{
    if (!mMapChangedCallback)
    {
        return;
    }

    const auto& tChanged = pMessage.response().map_changed();
    const auto& tPosition = tChanged.position();

    MapSnapshot tSnapshot{ static_cast<uint32_t>(tChanged.map_id()), tPosition.x(), tPosition.y(), tPosition.z(), tChanged.yaw() };

    mMapChangedCallback(tSnapshot);
}
