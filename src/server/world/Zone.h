#ifndef LAKOT_SERVER_ZONE_H
#define LAKOT_SERVER_ZONE_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio.hpp>

#include <connection.pb.h>

#include "NetworkSession.h"
#include "MovementRules.h"
#include "MapCatalog.h"

#include "WorldRegistry.h"
#include "CombatSystem.h"
#include "MonsterBrain.h"
#include "LootSystem.h"
#include "Monster.h"
#include "Npc.h"
#include "MonsterCatalog.h"

#include "../game/Character.h"
#include "../script/ScriptEngine.h"

namespace lakot
{

class ItemCatalog;

// One map's simulation, running on its own thread at a fixed tick.
//
// The threading rule this class exists to enforce: zone state (mRegistry and
// everything beside it) is touched ONLY from the zone's own thread. Network
// I/O threads never reach in - every public entry point below just posts work
// onto mContext, which is single threaded, so nothing inside needs a lock.
// That is why WorldRegistry could drop its shared_mutex.
//
// Zones do not call into each other either. Moving a player between maps goes
// through the TransferFunction callback (WorldController), which posts into
// the destination zone the same way the network layer does. Keeping that
// boundary a message even while both zones live in the same process is what
// makes moving a zone to its own process or machine later a deployment change
// rather than a rewrite.
class Zone
{
public:
    using Packet = NetworkSession<connection::Message>::Packet;

    // Delivers a finished packet to one character's session. Implemented by
    // WorldController against the SessionRegistry - a zone never touches a
    // session object itself.
    using SendFunction = std::function<void(uint64_t pCharacterId, Packet pPacket)>;

    // A player stepped into a portal and belongs to another map now - and
    // takes their whole character with them, which is why this carries a
    // CharacterSnapshot rather than just an id: the destination zone has to
    // reconstruct the character without going back to the database.
    using TransferFunction = std::function<void(const CharacterSnapshot& pCharacter,
                                                uint32_t pTargetMapId,
                                                const PlayerState& pSpawnState)>;

    // Everything about a character that needs to reach storage, gathered into
    // one value because it is always written as a unit and because the zone
    // thread must hand a COPY to the database thread - the live Character
    // stays behind, owned by this zone.
    struct PersistData
    {
        uint64_t characterId = 0;
        uint32_t mapId = 0;
        PlayerState position;
        CharacterStats stats;
        std::vector<ItemInstance> items;
    };

    // Writes a character through to storage - on leaving the zone, and on the
    // periodic autosave.
    using PersistFunction = std::function<void(const PersistData&)>;

    // pMap and pMapCatalog are immutable after startup, like pItemCatalog.
    Zone(const MapData& pMap, const MapCatalog& pMapCatalog, const ItemCatalog& pItemCatalog, const MonsterCatalog& pMonsterCatalog);
    ~Zone();

    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;

    void start(SendFunction pSend, TransferFunction pTransfer, PersistFunction pPersist,
               const std::string& pScriptDirectory);

    // Flushes every tracked position through PersistFunction, then drains the
    // remaining work and joins the thread.
    void stop();

    uint32_t getMapId() const;

    // ---- Called from other threads. Each one only posts; none of them read
    // or write zone state on the caller's thread. ----

    void enterPlayer(CharacterSnapshot pCharacter, const PlayerState& pState);
    void leavePlayer(uint64_t pCharacterId);

    // A client took its session back after a dropped connection. Resends
    // everything it would otherwise only have heard about incrementally: the
    // whole area of interest, its own position, and its inventory.
    void resumePlayer(uint64_t pCharacterId);

    void submitState(uint64_t pCharacterId, const PlayerState& pState);

    // Runs pAction against a character on THIS zone's thread. The only
    // sanctioned way for anything outside the zone to touch a Character:
    // handing in the work instead of taking out a reference is what keeps
    // "zone state is single threaded" true, and it is the shape the Lua
    // bindings will use too. Silently does nothing if the character is not
    // (or no longer) here.
    //
    // pAction returns true if it changed anything the client needs to see, in
    // which case an InventoryUpdate is pushed automatically. Putting that
    // here rather than at every call site is what stops a future mutation
    // path from silently forgetting to tell the client.
    void withCharacter(uint64_t pCharacterId, std::function<bool(Character&)> pAction);

    // Delivers pPacket to everyone currently inside pOriginCharacterId's area
    // of interest, excluding the origin. Used for nearby world chat, which is
    // sent immediately rather than batched into the next snapshot.
    void sendNearby(uint64_t pOriginCharacterId, Packet pPacket);

    void attack(uint64_t pCharacterId, float pYaw, uint32_t pCombo);

    void pickUp(uint64_t pCharacterId, uint64_t pEntityId, uint64_t pReplyTo);

private:
    // Simulation step. Movement is client-reported today, so this mostly
    // paces portal checks and the snapshot cadence, but it is the loop any
    // server-side simulation slots into.
    static constexpr int kTickIntervalMs = 50;                 // 20 Hz

    // Snapshots go out every other tick (10 Hz). Halves outbound traffic
    // versus one per tick, and matches the interval the client interpolates
    // remote players over.
    static constexpr uint32_t kSnapshotEveryNTicks = 2;

    // How often a still-connected player's position is written through, so a
    // server crash costs at most this much progress instead of everything
    // since login (positions used to be saved on disconnect only).
    static constexpr uint32_t kAutosaveEveryNTicks = 20 * 60;  // ~60 s

    // ---- Movement validation ----
    // The client walks at 5.0 units/second (WorldScene::update). The server
    // does not re-simulate movement - it accepts the position the client
    // reports, but only if the client could plausibly have walked there since
    // the last position it accepted. That closes speed hacking and teleporting
    // without needing client-side prediction and reconciliation.
    static constexpr float kMaxSpeed = MovementRules::kWalkSpeed;

    // Absorbs network jitter and the mismatch between the client's send
    // cadence and this tick - without it, an update that arrives slightly
    // early reads as moving slightly too fast.
    static constexpr float kPositionTolerance = 1.5f;

    static constexpr float kHeightTolerance = 0.5f;

    // Ceiling on how much movement budget can accumulate while a player sends
    // nothing. Without it, standing still for a minute would buy a 300-unit
    // teleport; with it, the longest legitimate stall still only grants a
    // couple of seconds of walking.
    static constexpr double kMaxBudgetSeconds = 2.0;

    static constexpr double kPickupCooldownSeconds = 0.1;

    const MapData& mMap;
    const MapCatalog& mMapCatalog;

    // Immutable for the life of the process (see ItemCatalog), so holding a
    // bare reference across threads is safe.
    const ItemCatalog& mItemCatalog;
    const MonsterCatalog& mMonsterCatalog;

    boost::asio::io_context mContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> mWorkGuard;
    boost::asio::steady_timer mTickTimer;
    std::thread mThread;

    // ---- Zone-thread-only state below this line. ----
    WorldRegistry mRegistry;

    // The characters this zone owns. Kept separate from mRegistry on purpose:
    // that is the spatial index and is walked densely every tick, so it has no
    // business carrying inventories around.
    std::unordered_map<uint64_t, Character> mCharacters;

    static constexpr uint64_t kWorldEntityIdBase = 1ull << 62;
    static constexpr double kTickSeconds = kTickIntervalMs / 1000.0;

    std::unordered_map<uint64_t, Monster> mMonsters;
    std::unordered_map<uint64_t, Npc> mNpcs;
    CombatSystem mCombat;
    MonsterBrain mMonsterBrain;
    double mTime{0.0};
    uint64_t mNextEntityId{kWorldEntityIdBase};
    LootSystem mLoot;
    std::unordered_map<uint64_t, double> mLastPickupAt;

    // One Lua state per zone, created ON the zone thread (see start()) so the
    // state is only ever touched by the thread that owns it. This is the
    // property that makes zone scripting genuinely parallel and lock-free,
    // and the reason Lua was picked over Python.
    std::unique_ptr<ScriptEngine> mScriptEngine;

    uint32_t mTickCounter{0};

    // When each character's last ACCEPTED position was recorded. A rejected
    // update deliberately does not advance this, so the movement budget keeps
    // accruing: a client whose packet was merely delayed gets accepted on its
    // next try, while one genuinely trying to move too fast stays capped at
    // kMaxSpeed on average.
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> mLastAcceptedAt;

    // Characters whose latest update was rejected and who therefore need to
    // be told, in the next snapshot, where the server actually thinks they
    // are (WorldSnapshot.self_correction).
    std::unordered_set<uint64_t> mPendingCorrections;

    std::unordered_set<uint64_t> mAppearanceChanged;

    SendFunction mSend;
    TransferFunction mTransfer;
    PersistFunction mPersist;

    void scheduleTick();
    void tick();

    void applyPortals();
    void sendSnapshots();

    // Pushes the character's whole inventory and stats. Called on entering
    // the world and after anything that changes either.
    void sendInventoryUpdate(const Character& pCharacter);

    // pForceAll false means "only characters marked dirty" - what the
    // periodic autosave wants, so an idle zone writes nothing at all.
    void persistAll(bool pForceAll);
    void persistCharacter(uint64_t pCharacterId);

    // Fills in an EntityState/EntitySpawn from the registry. Returns false if
    // the character is no longer on this map.
    bool fillEntityState(uint64_t pCharacterId, services::world::EntityState* pOut) const;
    bool fillEntitySpawn(uint64_t pCharacterId, services::world::EntitySpawn* pOut) const;

    void spawnWorldEntities();
    void respawnPlayer(uint64_t pCharacterId);
    void fillSelfVitals(const Character& pCharacter, services::world::SelfVitals* pOut) const;

    // Only items with a model; their look is everything other players need to know.
    std::vector<services::world::EquipmentVisual> getEquipmentVisuals(const Character& pCharacter) const;
};

}

#endif
