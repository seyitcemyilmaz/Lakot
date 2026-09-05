#ifndef LAKOT_WORLDCONTROLLER_H
#define LAKOT_WORLDCONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>

#include <connection.pb.h>

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class WorldController
{
public:
    struct PlayerSnapshot
    {
        uint64_t playerId;
        float x;
        float y;
        float z;
        float yaw;
        // Only populated on a join event (PlayerJoined) - state-update
        // broadcasts don't resend it, so it's empty on those; callers
        // should not overwrite an already-known username with this field
        // unless it's non-empty.
        std::string username;
    };

    struct MapSnapshot
    {
        uint32_t mapId;
        float x;
        float y;
        float z;
        float yaw;
    };

    using PlayerJoinedCallback = std::function<void(const PlayerSnapshot&)>;
    using PlayerStateUpdateCallback = std::function<void(const PlayerSnapshot&)>;
    using PlayerLeftCallback = std::function<void(uint64_t)>;
    using MapChangedCallback = std::function<void(const MapSnapshot&)>;

    WorldController(NetworkManager* pNetworkManager);

    void initialize();

    void sendPlayerStateUpdate(float pX, float pY, float pZ, float pYaw);

    void setPlayerJoinedCallback(PlayerJoinedCallback pCallback);
    void setPlayerStateUpdateCallback(PlayerStateUpdateCallback pCallback);
    void setPlayerLeftCallback(PlayerLeftCallback pCallback);
    void setMapChangedCallback(MapChangedCallback pCallback);

private:
    NetworkManager* mNetworkManager;

    PlayerJoinedCallback mPlayerJoinedCallback;
    PlayerStateUpdateCallback mPlayerStateUpdateCallback;
    PlayerLeftCallback mPlayerLeftCallback;
    MapChangedCallback mMapChangedCallback;

    void handlePlayerJoined(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePlayerStateBroadcast(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePlayerLeft(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleMapChanged(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
