#include "WorldRegistry.h"

#include <cmath>
#include <utility>

using namespace lakot;

namespace
{
    // Two signed cell coordinates packed into one key. Cast through the
    // unsigned type so negative coordinates (the world is centred on the
    // origin, so half of it is negative) pack and compare without any
    // implementation-defined sign behaviour.
    uint64_t packCell(int32_t pCellX, int32_t pCellZ)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(pCellX)) << 32)
             |  static_cast<uint64_t>(static_cast<uint32_t>(pCellZ));
    }
}

uint64_t WorldRegistry::cellKeyFor(float pX, float pZ)
{
    // std::floor, not truncation: truncating maps both -0.5 and +0.5 to cell
    // 0, which would make the cell straddling the origin twice as wide as
    // every other one and silently break the 3x3 sufficiency argument.
    return packCell(static_cast<int32_t>(std::floor(pX / kCellSize)),
                    static_cast<int32_t>(std::floor(pZ / kCellSize)));
}

void WorldRegistry::insertIntoCell(uint64_t pCharacterId, uint64_t pCellKey)
{
    mCells[pCellKey].insert(pCharacterId);
}

void WorldRegistry::removeFromCell(uint64_t pCharacterId, uint64_t pCellKey)
{
    auto tIterator = mCells.find(pCellKey);

    if (tIterator == mCells.end())
    {
        return;
    }

    tIterator->second.erase(pCharacterId);

    // Dropping emptied cells keeps mCells proportional to occupancy rather
    // than to how much of the map anyone has ever walked across.
    if (tIterator->second.empty())
    {
        mCells.erase(tIterator);
    }
}

void WorldRegistry::add(uint64_t pCharacterId, std::string pName, const PlayerState& pState, bool pIsViewer)
{
    // Re-entering an id that is somehow still filed (a relogin racing its own
    // cleanup) must not leave a stale cell membership behind.
    auto tExisting = mPlayers.find(pCharacterId);

    if (tExisting != mPlayers.end())
    {
        removeFromCell(pCharacterId, tExisting->second.cellKey);
    }

    PlayerEntry tEntry;
    tEntry.state = pState;
    tEntry.name = std::move(pName);
    tEntry.isDirty = true;
    tEntry.isViewer = pIsViewer;
    tEntry.cellKey = cellKeyFor(pState.x, pState.z);

    insertIntoCell(pCharacterId, tEntry.cellKey);

    mPlayers[pCharacterId] = std::move(tEntry);

    if (pIsViewer)
    {
        mVisibility[pCharacterId];
    }
}

void WorldRegistry::remove(uint64_t pCharacterId)
{
    auto tIterator = mPlayers.find(pCharacterId);

    if (tIterator != mPlayers.end())
    {
        removeFromCell(pCharacterId, tIterator->second.cellKey);
        mPlayers.erase(tIterator);
    }

    // Leaving the map is reported to the watchers by the next computeDeltas()
    // (the id simply stops appearing in their recomputed sets), so nothing
    // needs to be pushed here - only this player's own view is dropped.
    mVisibility.erase(pCharacterId);
}

void WorldRegistry::setState(uint64_t pCharacterId, const PlayerState& pState)
{
    auto tIterator = mPlayers.find(pCharacterId);

    if (tIterator == mPlayers.end())
    {
        return;
    }

    uint64_t tNewCellKey = cellKeyFor(pState.x, pState.z);

    if (tNewCellKey != tIterator->second.cellKey)
    {
        removeFromCell(pCharacterId, tIterator->second.cellKey);
        insertIntoCell(pCharacterId, tNewCellKey);
        tIterator->second.cellKey = tNewCellKey;
    }

    tIterator->second.state = pState;
    tIterator->second.isDirty = true;
}

void WorldRegistry::resetView(uint64_t pCharacterId)
{
    auto tIterator = mVisibility.find(pCharacterId);

    if (tIterator != mVisibility.end())
    {
        tIterator->second.clear();
    }
}

bool WorldRegistry::contains(uint64_t pCharacterId) const
{
    return mPlayers.find(pCharacterId) != mPlayers.end();
}

const PlayerState* WorldRegistry::getState(uint64_t pCharacterId) const
{
    auto tIterator = mPlayers.find(pCharacterId);
    return tIterator == mPlayers.end() ? nullptr : &tIterator->second.state;
}

const std::string* WorldRegistry::getName(uint64_t pCharacterId) const
{
    auto tIterator = mPlayers.find(pCharacterId);
    return tIterator == mPlayers.end() ? nullptr : &tIterator->second.name;
}

std::vector<uint64_t> WorldRegistry::getPlayerIds() const
{
    std::vector<uint64_t> tIds;
    tIds.reserve(mPlayers.size());

    for (const auto& [tId, tEntry] : mPlayers)
    {
        if (tEntry.isViewer)
        {
            tIds.push_back(tId);
        }
    }

    return tIds;
}

std::vector<uint64_t> WorldRegistry::getNearbyPlayerIds(uint64_t pCharacterId) const
{
    auto tIterator = mVisibility.find(pCharacterId);

    if (tIterator == mVisibility.end())
    {
        return {};
    }

    std::vector<uint64_t> tIds;

    for (uint64_t tId : tIterator->second)
    {
        auto tEntry = mPlayers.find(tId);

        if (tEntry != mPlayers.end() && tEntry->second.isViewer)
        {
            tIds.push_back(tId);
        }
    }

    return tIds;
}

void WorldRegistry::queryRadius(float pX, float pZ, float pRadius, std::vector<uint64_t>& pResult) const
{
    pResult.clear();

    int32_t tMinX = static_cast<int32_t>(std::floor((pX - pRadius) / kCellSize));
    int32_t tMaxX = static_cast<int32_t>(std::floor((pX + pRadius) / kCellSize));
    int32_t tMinZ = static_cast<int32_t>(std::floor((pZ - pRadius) / kCellSize));
    int32_t tMaxZ = static_cast<int32_t>(std::floor((pZ + pRadius) / kCellSize));

    for (int32_t tCellZ = tMinZ; tCellZ <= tMaxZ; ++tCellZ)
    {
        for (int32_t tCellX = tMinX; tCellX <= tMaxX; ++tCellX)
        {
            auto tCell = mCells.find(packCell(tCellX, tCellZ));

            if (tCell == mCells.end())
            {
                continue;
            }

            for (uint64_t tId : tCell->second)
            {
                const PlayerState& tState = mPlayers.at(tId).state;
                float tDx = tState.x - pX;
                float tDz = tState.z - pZ;

                if (tDx * tDx + tDz * tDz <= pRadius * pRadius)
                {
                    pResult.push_back(tId);
                }
            }
        }
    }
}

void WorldRegistry::queryNearby(uint64_t pCharacterId, const PlayerState& pState, std::unordered_set<uint64_t>& pResult) const
{
    pResult.clear();

    int32_t tCellX = static_cast<int32_t>(std::floor(pState.x / kCellSize));
    int32_t tCellZ = static_cast<int32_t>(std::floor(pState.z / kCellSize));

    // The grid is 2D (x/z) while the exact test below is 3D. That is
    // deliberate: players are distributed across the ground plane, so binning
    // by height would add a dimension that almost never separates anyone,
    // and any y difference is still caught by the radius test.
    for (int32_t tOffsetZ = -1; tOffsetZ <= 1; ++tOffsetZ)
    {
        for (int32_t tOffsetX = -1; tOffsetX <= 1; ++tOffsetX)
        {
            auto tCellIterator = mCells.find(packCell(tCellX + tOffsetX, tCellZ + tOffsetZ));

            if (tCellIterator == mCells.end())
            {
                continue;
            }

            for (uint64_t tOtherId : tCellIterator->second)
            {
                if (tOtherId == pCharacterId)
                {
                    continue;
                }

                auto tOtherIterator = mPlayers.find(tOtherId);

                if (tOtherIterator == mPlayers.end())
                {
                    continue;
                }

                const PlayerState& tOtherState = tOtherIterator->second.state;

                float tDx = pState.x - tOtherState.x;
                float tDy = pState.y - tOtherState.y;
                float tDz = pState.z - tOtherState.z;

                // Narrow phase - the cell neighbourhood is a superset of the
                // circle, so this is what actually defines the radius.
                if (tDx * tDx + tDy * tDy + tDz * tDz <= kInterestRadiusSq)
                {
                    pResult.insert(tOtherId);
                }
            }
        }
    }
}

std::unordered_map<uint64_t, VisibilityDelta> WorldRegistry::computeDeltas()
{
    std::unordered_map<uint64_t, VisibilityDelta> tResult;

    std::unordered_set<uint64_t> tNewNearby;

    for (const auto& [tCharacterId, tEntry] : mPlayers)
    {
        if (!tEntry.isViewer)
        {
            continue;
        }

        queryNearby(tCharacterId, tEntry.state, tNewNearby);

        std::unordered_set<uint64_t>& tOldNearby = mVisibility[tCharacterId];

        VisibilityDelta tDelta;

        for (uint64_t tOtherId : tNewNearby)
        {
            if (tOldNearby.find(tOtherId) == tOldNearby.end())
            {
                tDelta.entered.push_back(tOtherId);
            }
            else
            {
                // Already visible last tick - only worth resending if it
                // actually moved. This is what keeps a crowd of idle players
                // from costing anything.
                auto tOtherIterator = mPlayers.find(tOtherId);

                if (tOtherIterator != mPlayers.end() && tOtherIterator->second.isDirty)
                {
                    tDelta.moved.push_back(tOtherId);
                }
            }
        }

        for (uint64_t tOtherId : tOldNearby)
        {
            if (tNewNearby.find(tOtherId) == tNewNearby.end())
            {
                tDelta.left.push_back(tOtherId);
            }
        }

        tOldNearby = tNewNearby;

        if (!tDelta.isEmpty())
        {
            tResult.emplace(tCharacterId, std::move(tDelta));
        }
    }

    // Cleared only after every player's delta has been built - a dirty flag
    // has to stay set until everyone who can see that entity has been told.
    for (auto& [tCharacterId, tEntry] : mPlayers)
    {
        tEntry.isDirty = false;
    }

    return tResult;
}
