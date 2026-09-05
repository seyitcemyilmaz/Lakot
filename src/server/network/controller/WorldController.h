#ifndef LAKOT_SERVER_WORLDCONTROLLER_H
#define LAKOT_SERVER_WORLDCONTROLLER_H

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <connection.pb.h>

#include "BaseController.h"
#include "../../world/WorldRegistry.h"
#include "../../world/MapCatalog.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

class WorldController : public BaseController
{
public:
    WorldController(NetworkManager& pNetworkManager, RepositoryManager& pRepositoryManager);

    void initialize() override;

    // Called from AuthController's successful-login branch, before the
    // login response goes out - pre-assigns a returning player's map from
    // their restored state so their first PlayerStateUpdate routes to the
    // correct registry instead of defaulting to Town.
    void seedPlayerMap(uint64_t pUserId, services::world::MapId pMapId);

    // Also called from AuthController at login, alongside seedPlayerMap -
    // caches the username so PlayerJoined pushes can include it without an
    // extra DB round-trip.
    void setPlayerUsername(uint64_t pUserId, const std::string& pUsername);

    std::string getPlayerUsername(uint64_t pUserId);

    // Used by ChatController to resolve a whisper's "message_to" username
    // into an online player's id - nullopt if nobody by that name is
    // currently logged in.
    std::optional<uint64_t> findOnlinePlayerIdByUsername(const std::string& pUsername);

private:
    // One registry per map - a player moving between maps is just removed
    // from one and inserted into another (existing leave/join paths).
    std::unordered_map<uint32_t, WorldRegistry> mRegistries;

    // Guards all three maps below - small, only touched at login/disconnect/
    // portal time (never per movement update), so sharing one lock is simplest.
    std::mutex mPlayerMetaMutex;
    std::unordered_map<uint64_t, uint32_t> mPlayerMapAssignment;
    std::unordered_map<uint64_t, std::string> mPlayerUsernames;
    std::unordered_map<std::string, uint64_t> mUsernameToUserId;

    void handlePlayerStateUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePlayerDisconnect(uint64_t pUserId);

    services::world::MapId getOrAssignPlayerMap(uint64_t pUserId);
    WorldRegistry& getRegistryForMap(services::world::MapId pMapId);

    void teleportPlayer(uint64_t pUserId, services::world::MapId pFromMapId, const PortalDefinition& pPortal);

    void sendPlayerJoined(uint64_t pTargetUserId, uint64_t pPlayerId, const PlayerState& pState);
    void sendPlayerLeft(uint64_t pTargetUserId, uint64_t pPlayerId);
    void sendMapChanged(uint64_t pTargetUserId, services::world::MapId pMapId, const PlayerState& pState);
};

}

#endif
