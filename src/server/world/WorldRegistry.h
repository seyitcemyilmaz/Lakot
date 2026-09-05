#ifndef LAKOT_SERVER_WORLDREGISTRY_H
#define LAKOT_SERVER_WORLDREGISTRY_H

#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace lakot
{

struct PlayerState
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

struct VisibilityChange
{
    std::vector<uint64_t> entered;
    std::vector<uint64_t> exited;
    std::vector<uint64_t> stillNearby;
};

// Tracks every connected player's last known position and, on each update,
// works out who newly entered or left interest range of the mover - never a
// global broadcast. The distance scan in updatePlayer() is the one place a
// future spatial-grid/quadtree would replace; everything else (network
// plumbing) stays the same.
class WorldRegistry
{
public:
    VisibilityChange updatePlayer(uint64_t pUserId, const PlayerState& pState);

    // Removes pUserId and returns the ids of everyone who still had it in view.
    std::vector<uint64_t> removePlayer(uint64_t pUserId);

    std::optional<PlayerState> getState(uint64_t pUserId) const;

private:
    static constexpr float kInterestRadiusSq = 50.0f * 50.0f;

    mutable std::shared_mutex mMutex;
    std::unordered_map<uint64_t, PlayerState> mPlayers;
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> mVisibility;

    bool isWithinInterestRange(const PlayerState& pA, const PlayerState& pB) const;
};

}

#endif
