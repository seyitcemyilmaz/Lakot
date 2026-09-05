#include "WorldRegistry.h"

using namespace lakot;

VisibilityChange WorldRegistry::updatePlayer(uint64_t pUserId, const PlayerState& pState)
{
    std::unique_lock tLock(mMutex);

    mPlayers[pUserId] = pState;

    std::unordered_set<uint64_t> tNewNearby;

    for (const auto& [tOtherId, tOtherState] : mPlayers)
    {
        if (tOtherId == pUserId)
        {
            continue;
        }

        if (isWithinInterestRange(pState, tOtherState))
        {
            tNewNearby.insert(tOtherId);
        }
    }

    // Reference into the map - stays valid across the operator[] insertions
    // below (unordered_map only invalidates references on erase, not on
    // insert/rehash), and is overwritten with the new set at the end.
    std::unordered_set<uint64_t>& tOldNearby = mVisibility[pUserId];

    VisibilityChange tChange;

    for (uint64_t tOtherId : tNewNearby)
    {
        if (tOldNearby.find(tOtherId) == tOldNearby.end())
        {
            tChange.entered.push_back(tOtherId);
        }
        else
        {
            tChange.stillNearby.push_back(tOtherId);
        }
    }

    for (uint64_t tOtherId : tOldNearby)
    {
        if (tNewNearby.find(tOtherId) == tNewNearby.end())
        {
            tChange.exited.push_back(tOtherId);
        }
    }

    // Keep both sides of every entered/exited pair symmetric, so a
    // stationary player's own next update still diffs correctly even
    // though it never moved itself.
    for (uint64_t tOtherId : tChange.entered)
    {
        mVisibility[tOtherId].insert(pUserId);
    }

    for (uint64_t tOtherId : tChange.exited)
    {
        mVisibility[tOtherId].erase(pUserId);
    }

    tOldNearby = std::move(tNewNearby);

    return tChange;
}

std::vector<uint64_t> WorldRegistry::removePlayer(uint64_t pUserId)
{
    std::unique_lock tLock(mMutex);

    std::vector<uint64_t> tWatchers;

    auto tVisibilityIterator = mVisibility.find(pUserId);

    if (tVisibilityIterator != mVisibility.end())
    {
        tWatchers.assign(tVisibilityIterator->second.begin(), tVisibilityIterator->second.end());

        for (uint64_t tWatcherId : tWatchers)
        {
            auto tWatcherVisibility = mVisibility.find(tWatcherId);

            if (tWatcherVisibility != mVisibility.end())
            {
                tWatcherVisibility->second.erase(pUserId);
            }
        }

        mVisibility.erase(tVisibilityIterator);
    }

    mPlayers.erase(pUserId);

    return tWatchers;
}

std::optional<PlayerState> WorldRegistry::getState(uint64_t pUserId) const
{
    std::shared_lock tLock(mMutex);

    auto tIterator = mPlayers.find(pUserId);

    if (tIterator == mPlayers.end())
    {
        return std::nullopt;
    }

    return tIterator->second;
}

bool WorldRegistry::isWithinInterestRange(const PlayerState& pA, const PlayerState& pB) const
{
    float tDx = pA.x - pB.x;
    float tDy = pA.y - pB.y;
    float tDz = pA.z - pB.z;

    float tDistanceSq = tDx * tDx + tDy * tDy + tDz * tDz;

    return tDistanceSq <= kInterestRadiusSq;
}
