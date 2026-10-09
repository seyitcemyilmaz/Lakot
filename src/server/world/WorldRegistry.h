#ifndef LAKOT_SERVER_WORLDREGISTRY_H
#define LAKOT_SERVER_WORLDREGISTRY_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lakot
{

struct PlayerState
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

// What changed inside one player's area of interest since the previous tick.
// `moved` lists only entities that actually changed position, and never
// overlaps `entered` (a newly visible entity is announced once, with its full
// spawn data, not also as a move in the same tick).
struct VisibilityDelta
{
    std::vector<uint64_t> entered;
    std::vector<uint64_t> moved;
    std::vector<uint64_t> left;

    bool isEmpty() const
    {
        return entered.empty() && moved.empty() && left.empty();
    }
};

// Every player on ONE map, plus who can currently see whom.
//
// Deliberately NOT thread safe, and that is the point: it is owned by exactly
// one Zone and only ever touched from that Zone's own thread (see Zone.h).
// The previous version guarded itself with a shared_mutex because it was
// reached directly from whichever Asio I/O thread handled an incoming packet
// - which meant every movement update on a map serialized through one lock,
// so the worker pool bought no parallelism for simulation at all.
class WorldRegistry
{
public:
    static constexpr float kInterestRadius = 75.0f;

    void add(uint64_t pCharacterId, std::string pName, const PlayerState& pState, bool pIsViewer = true);
    void remove(uint64_t pCharacterId);

    // Records a new position. Marks the entity dirty so the next
    // computeDeltas() reports it as moved to everyone who can see it.
    void setState(uint64_t pCharacterId, const PlayerState& pState);

    bool contains(uint64_t pCharacterId) const;

    // Forgets what pCharacterId has already been told about, so the next
    // computeDeltas() reports everyone in range to it as entered - used when
    // a client resumes a session and starts again from an empty view.
    void resetView(uint64_t pCharacterId);

    // nullptr if not on this map. The pointer is valid until the next
    // add()/remove(), which - being single threaded - callers control.
    const PlayerState* getState(uint64_t pCharacterId) const;
    const std::string* getName(uint64_t pCharacterId) const;

    std::vector<uint64_t> getPlayerIds() const;

    // Who pCharacterId can currently see, as of the last computeDeltas().
    // Reused for nearby ("!"-less) chat rather than recomputing distances.
    std::vector<uint64_t> getNearbyPlayerIds(uint64_t pCharacterId) const;

    void queryRadius(float pX, float pZ, float pRadius, std::vector<uint64_t>& pResult) const;

    // Recomputes every player's area of interest and returns, per player,
    // what changed since the previous call. Called exactly once per snapshot
    // tick. Players whose view did not change at all are omitted from the
    // result, so a quiet zone produces no outbound messages.
    std::unordered_map<uint64_t, VisibilityDelta> computeDeltas();

private:
    struct PlayerEntry
    {
        PlayerState state;
        std::string name;

        // Position changed since the last computeDeltas(). Cleared at the end
        // of each one, so a standing-still player is not re-sent every tick.
        bool isDirty = true;
        bool isViewer = true;

        // Which grid cell this entry is currently filed under. Cached so
        // setState() only touches mCells on an actual cell crossing, which
        // for a walking player is rare compared to how often it is called.
        uint64_t cellKey = 0;
    };

    static constexpr float kInterestRadiusSq = kInterestRadius * kInterestRadius;

    // Cell size equal to the interest radius is what makes a 3x3 neighbourhood
    // both sufficient and minimal: wherever inside its own cell a player
    // stands, a circle of radius R around it cannot reach past the cells
    // immediately adjacent, so scanning 3x3 can never miss anyone - and no
    // smaller neighbourhood is safe.
    static constexpr float kCellSize = kInterestRadius;

    std::unordered_map<uint64_t, PlayerEntry> mPlayers;
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> mVisibility;

    // Broad phase: cell key -> the characters standing in that cell. Empty
    // cells are erased rather than kept, so this stays proportional to how
    // many cells are actually occupied, not to the size of the map.
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> mCells;

    static uint64_t cellKeyFor(float pX, float pZ);

    void insertIntoCell(uint64_t pCharacterId, uint64_t pCellKey);
    void removeFromCell(uint64_t pCharacterId, uint64_t pCellKey);

    // The one place "who is near this player" is answered. Broad phase over
    // the 3x3 cell neighbourhood, then the exact radius test - so the cost of
    // one query is proportional to the number of nearby players, not to the
    // number of players on the map. That is the difference between O(N) and
    // O(neighbours) per player per tick, i.e. between O(N^2) and O(N) for a
    // whole tick.
    void queryNearby(uint64_t pCharacterId, const PlayerState& pState, std::unordered_set<uint64_t>& pResult) const;
};

}

#endif
