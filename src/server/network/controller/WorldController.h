#ifndef LAKOT_SERVER_WORLDCONTROLLER_H
#define LAKOT_SERVER_WORLDCONTROLLER_H

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <connection.pb.h>

#include "BaseController.h"
#include "../../world/Zone.h"
#include "../../world/WorldRegistry.h"
#include "MapCatalog.h"

namespace lakot
{

template <typename MessageType>
class NetworkSession;

// Routes network traffic to the right Zone and back out to the right session.
//
// It owns no world state of its own beyond the bookkeeping needed to do that
// routing: which zone a character is currently in, and their name. All actual
// simulation lives inside the zones, each on its own thread (see Zone.h), so
// this class only ever posts into them and never reads their state.
class WorldController : public BaseController
{
public:
    WorldController(NetworkManager& pNetworkManager,
                    RepositoryManager& pRepositoryManager,
                    const ItemCatalog& pItemCatalog,
                    const MapCatalog& pMapCatalog,
                    const MonsterCatalog& pMonsterCatalog);

    void initialize() override;

    // Stops every zone, which flushes all tracked positions to storage on the
    // way out. Called from Server's destructor before the database pool is
    // torn down.
    void shutdown();

    // Called from CharacterController once a character has been loaded: puts it
    // into its stored map's zone and caches the name used for nameplates and
    // whisper lookups.
    void enterWorld(const CharacterSnapshot& pCharacter,
                    uint32_t pMapId,
                    const PlayerState& pState);

    // See Zone::resumePlayer.
    void resumeCharacter(uint64_t pCharacterId);


    std::string getCharacterName(uint64_t pCharacterId);

    // Resolves a whisper's "message_to" name to an online character id -
    // nullopt if nobody by that name is currently logged in.
    std::optional<uint64_t> findOnlineCharacterIdByName(const std::string& pName);

    // Hands pPacket to the sender's zone for delivery to everyone in their
    // area of interest. Used by ChatController for nearby ("!"-less) world
    // chat. The zone answers "who is nearby" on its own thread; nothing here
    // reads that state.
    void sendNearby(uint64_t pOriginCharacterId, Zone::Packet pPacket);

    // Runs pAction against a character on whichever zone thread owns it - see
    // Zone::withCharacter. Returning true from pAction makes the zone push an
    // InventoryUpdate. The single entry point for anything outside the world
    // layer (InventoryController today, quest scripts later) that needs to
    // change a character.
    void withCharacter(uint64_t pCharacterId, std::function<bool(Character&)> pAction);

private:
    // Zones are constructed in initialize(), not the constructor, because
    // each needs the ItemCatalog and that is only filled once the database is
    // up - see Server::initialize().
    std::unordered_map<uint32_t, std::unique_ptr<Zone>> mZones;

    const ItemCatalog& mItemCatalog;
    const MapCatalog& mMapCatalog;
    const MonsterCatalog& mMonsterCatalog;

    const std::string mScriptDirectory{"scripts"};

    // Guards both maps below. Only touched at login/disconnect/portal time,
    // never per movement update, so one lock is simplest.
    std::mutex mPlayerMetaMutex;
    std::unordered_map<uint64_t, uint32_t> mCharacterZone;
    std::unordered_map<uint64_t, std::string> mCharacterNames;
    std::unordered_map<std::string, uint64_t> mNameToCharacterId;

    void handlePlayerStateUpdate(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleAttackRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePickupItemRequest(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePlayerDisconnect(uint64_t pCharacterId);

    Zone* getZone(uint32_t pMapId);
    std::optional<uint32_t> getCharacterZone(uint64_t pCharacterId);

    // The Zone::TransferFunction implementation - moves a character between
    // two zones and tells their client about the new map.
    void transferCharacter(const CharacterSnapshot& pCharacter,
                           uint32_t pTargetMapId,
                           const PlayerState& pSpawnState);

    void sendToCharacter(uint64_t pCharacterId, Zone::Packet pPacket);
    void sendMapChanged(uint64_t pCharacterId, uint32_t pMapId, const PlayerState& pState);
};

}

#endif
