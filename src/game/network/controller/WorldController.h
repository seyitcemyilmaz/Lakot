#ifndef LAKOT_WORLDCONTROLLER_H
#define LAKOT_WORLDCONTROLLER_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <connection.pb.h>

#include "../GameTypes.h"

namespace lakot
{

class NetworkManager;

template <typename MessageType>
class NetworkSession;

class WorldController
{
public:
    struct EntitySnapshot
    {
        uint64_t entityId;
        float x;
        float y;
        float z;
        float yaw;

        // Only populated for entities that just entered view - a move does
        // not repeat identity data every tick, so it is empty there.
        std::string name;
        std::vector<EquippedVisual> equipment;

        uint32_t kind = 0;
        uint32_t templateId = 0;
        std::string model;
        uint32_t healthPercent = 100;
        uint32_t kingdom = 0;
        bool isDead = false;
        uint32_t count = 0;
        uint64_t gold = 0;
        uint32_t level = 0;
    };

    struct HitSnapshot
    {
        uint64_t targetId;
        uint32_t damage;
        bool isKilled;
    };

    struct CombatSnapshot
    {
        uint64_t attackerId;
        uint32_t combo;
        std::vector<HitSnapshot> hits;
    };

    struct VitalsSnapshot
    {
        uint64_t entityId;
        uint32_t healthPercent;
        bool isDead;
    };

    struct SelfVitalsSnapshot
    {
        int32_t health;
        int32_t maxHealth;
        uint32_t level;
        uint64_t experience;
        uint64_t experienceForNextLevel;
        float respawnSeconds;
        uint32_t kingdom;
    };

    struct AppearanceSnapshot
    {
        uint64_t entityId;
        std::vector<EquippedVisual> equipment;
    };

    // One server tick's worth of changes inside this client's area of
    // interest. Replaces the old one-message-per-event PlayerJoined /
    // PlayerLeft / PlayerStateBroadcast pushes: the server now batches a
    // whole tick into a single message per player, which is what stops
    // outbound traffic scaling with the square of the player count.
    struct WorldSnapshot
    {
        uint32_t tick;
        std::vector<EntitySnapshot> entered;
        std::vector<EntitySnapshot> moved;
        std::vector<uint64_t> left;

        // Present only when the SERVER repositioned this client itself
        // (rejected movement), meaning the local character must snap to it.
        std::optional<EntitySnapshot> selfCorrection;

        std::vector<AppearanceSnapshot> appearance;

        std::vector<CombatSnapshot> combat;
        std::vector<VitalsSnapshot> vitals;
        std::optional<SelfVitalsSnapshot> selfVitals;
    };

    struct MapSnapshot
    {
        uint32_t mapId;
        float x;
        float y;
        float z;
        float yaw;
    };

    using WorldSnapshotCallback = std::function<void(const WorldSnapshot&)>;
    using MapChangedCallback = std::function<void(const MapSnapshot&)>;
    using PickupResultCallback = std::function<void(services::world::PickupResult)>;

    WorldController(NetworkManager* pNetworkManager);

    void initialize();

    void sendPlayerStateUpdate(float pX, float pY, float pZ, float pYaw);
    void sendAttack(float pYaw, uint32_t pCombo);
    void sendPickup(uint64_t pEntityId);

    void setWorldSnapshotCallback(WorldSnapshotCallback pCallback);
    void setMapChangedCallback(MapChangedCallback pCallback);
    void setPickupResultCallback(PickupResultCallback pCallback);

    // Drops every callback above. They capture the WorldScene that registered
    // them, but this controller lives as long as the process (it is owned by
    // Engine's NetworkManager) - so a scene torn down without clearing them
    // leaves dangling `this` pointers here. Called from WorldScene::exit().
    void clearCallbacks();

private:
    NetworkManager* mNetworkManager;

    WorldSnapshotCallback mWorldSnapshotCallback;
    MapChangedCallback mMapChangedCallback;
    PickupResultCallback mPickupResultCallback;

    void handleWorldSnapshot(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handleMapChanged(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
    void handlePickupItemResponse(std::shared_ptr<NetworkSession<connection::Message>> pSession, const connection::Message& pMessage);
};

}

#endif
