#include "Pathfinder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>

using namespace lakot;

namespace
{
    constexpr float kDiagonalCost = 1.41421356f;

    // Road cells cost this much per step, so a walker takes a road when the
    // detour is modest - but never a long way round just to be on one.
    constexpr float kRoadCostFactor = 0.6f;

    // The heuristic scaled to the cheapest possible step (a road step), which
    // keeps it admissible: the search then really does prefer a road when it
    // is cheaper, instead of settling for the first straight-ish route.
    constexpr float kHeuristicFactor = kRoadCostFactor;

    // Expansion budget for that road-aware search.
    constexpr int kRoadAwareExpansions = 60000;

    // A blocked destination first looks this far around itself (in cells)
    // for somewhere walkable, then falls back to the walkable point nearest
    // it on the way back towards the walker.
    constexpr int kSnapRadius = 16;

    // Legs are checked finer than the server's 1-unit sampling, so a leg the
    // client accepts can never be one the server refuses.
    constexpr float kLegSampleStep = 0.25f;

    struct OpenEntry
    {
        float priority;
        int32_t cell;

        bool operator>(const OpenEntry& pOther) const
        {
            return priority > pOther.priority;
        }
    };

    // Per-thread search state sized to the map, reused between searches.
    // A cell's entries are only valid when its stamp matches the current
    // search, so nothing is cleared between calls.
    struct SearchBuffers
    {
        std::vector<uint32_t> stamp;
        std::vector<float> cost;
        std::vector<int32_t> parent;
        std::vector<uint8_t> isClosed;
        uint32_t current = 0;

        void begin(size_t pCellCount)
        {
            if (stamp.size() != pCellCount)
            {
                stamp.assign(pCellCount, 0);
                cost.resize(pCellCount);
                parent.resize(pCellCount);
                isClosed.resize(pCellCount);
                current = 0;
            }

            ++current;
        }

        void touch(int32_t pCell)
        {
            if (stamp[pCell] != current)
            {
                stamp[pCell] = current;
                cost[pCell] = std::numeric_limits<float>::max();
                parent[pCell] = -1;
                isClosed[pCell] = 0;
            }
        }
    };

    float octile(int pDx, int pDz)
    {
        int tAx = std::abs(pDx);
        int tAz = std::abs(pDz);
        return static_cast<float>(std::max(tAx, tAz)) + (kDiagonalCost - 1.0f) * static_cast<float>(std::min(tAx, tAz));
    }

    // Moves (pCellX, pCellZ) onto a walkable cell: the nearest one around it,
    // or else the first one on the straight line back towards the walker.
    bool snapToWalkable(const MapData& pMap, int& pCellX, int& pCellZ, int pTowardsX, int pTowardsZ)
    {
        if (pMap.isCellWalkable(pCellX, pCellZ))
        {
            return true;
        }

        for (int tRadius = 1; tRadius <= kSnapRadius; ++tRadius)
        {
            int tBestX = 0;
            int tBestZ = 0;
            int tBestDistance = std::numeric_limits<int>::max();

            for (int tDz = -tRadius; tDz <= tRadius; ++tDz)
            {
                for (int tDx = -tRadius; tDx <= tRadius; ++tDx)
                {
                    if (std::max(std::abs(tDx), std::abs(tDz)) != tRadius)
                    {
                        continue; // only the ring itself
                    }

                    if (pMap.isCellWalkable(pCellX + tDx, pCellZ + tDz) && tDx * tDx + tDz * tDz < tBestDistance)
                    {
                        tBestDistance = tDx * tDx + tDz * tDz;
                        tBestX = pCellX + tDx;
                        tBestZ = pCellZ + tDz;
                    }
                }
            }

            if (tBestDistance != std::numeric_limits<int>::max())
            {
                pCellX = tBestX;
                pCellZ = tBestZ;
                return true;
            }
        }

        // Deep inside something large (a lake, the mountain rim): walk the
        // line back towards the walker and stop at the first open cell.
        int tDx = pTowardsX - pCellX;
        int tDz = pTowardsZ - pCellZ;
        int tSteps = std::max(std::abs(tDx), std::abs(tDz));

        for (int tStep = 1; tStep <= tSteps; ++tStep)
        {
            int tX = pCellX + static_cast<int>(std::lround(static_cast<float>(tDx) * tStep / tSteps));
            int tZ = pCellZ + static_cast<int>(std::lround(static_cast<float>(tDz) * tStep / tSteps));

            if (pMap.isCellWalkable(tX, tZ))
            {
                pCellX = tX;
                pCellZ = tZ;
                return true;
            }
        }

        return false;
    }

    bool isLegOnRoad(const MapData& pMap, const glm::vec2& pFrom, const glm::vec2& pTo)
    {
        float tLength = glm::distance(pFrom, pTo);
        int tSteps = std::max(1, static_cast<int>(std::ceil(tLength / pMap.getCellSize())));

        for (int tStep = 0; tStep <= tSteps; ++tStep)
        {
            glm::vec2 tPoint = pFrom + (pTo - pFrom) * (static_cast<float>(tStep) / tSteps);

            if (!pMap.isCellRoad(pMap.getCellIndex(tPoint.x), pMap.getCellIndex(tPoint.y)))
            {
                return false;
            }
        }

        return true;
    }
}

std::vector<glm::vec2> Pathfinder::findPath(const MapData& pMap, const glm::vec2& pFrom, const glm::vec2& pTo)
{
    const int tResolution = pMap.getCellResolution();

    int tStartX = pMap.getCellIndex(pFrom.x);
    int tStartZ = pMap.getCellIndex(pFrom.y);
    int tGoalX = pMap.getCellIndex(pTo.x);
    int tGoalZ = pMap.getCellIndex(pTo.y);

    // An exact click on open ground is walked to exactly; anything else ends
    // at the centre of the chosen walkable cell.
    glm::vec2 tGoalPoint = pTo;

    if (!pMap.isWalkable(pTo.x, pTo.y) || !pMap.isCellWalkable(tGoalX, tGoalZ))
    {
        if (!snapToWalkable(pMap, tGoalX, tGoalZ, tStartX, tStartZ))
        {
            return {};
        }

        tGoalPoint = glm::vec2(pMap.getCellCenterX(tGoalX), pMap.getCellCenterZ(tGoalZ));
    }

    auto toIndex = [tResolution](int pX, int pZ) { return static_cast<int32_t>(pZ * tResolution + pX); };

    const int32_t tStart = toIndex(tStartX, tStartZ);
    const int32_t tGoal = toIndex(tGoalX, tGoalZ);

    thread_local SearchBuffers tBuffers;

    static const int kNeighbours[8][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };

    // One A* pass. Returns whether the goal was reached; pEnd is the goal or,
    // failing that, the explored cell nearest to it.
    auto search = [&](float pHeuristicFactor, int pMaxExpansions, int32_t& pEnd) -> bool
    {
        tBuffers.begin(static_cast<size_t>(tResolution) * tResolution);

        std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> tOpen;

        tBuffers.touch(tStart);
        tBuffers.cost[tStart] = 0.0f;
        tOpen.push({ 0.0f, tStart });

        int32_t tBest = tStart;
        float tBestDistance = octile(tGoalX - tStartX, tGoalZ - tStartZ);

        int tExpansions = 0;

        while (!tOpen.empty() && tExpansions < pMaxExpansions)
        {
            int32_t tCurrent = tOpen.top().cell;
            tOpen.pop();

            if (tBuffers.isClosed[tCurrent])
            {
                continue; // a stale queue entry
            }

            tBuffers.isClosed[tCurrent] = 1;
            ++tExpansions;

            int tX = tCurrent % tResolution;
            int tZ = tCurrent / tResolution;

            float tDistance = octile(tGoalX - tX, tGoalZ - tZ);

            if (tDistance < tBestDistance)
            {
                tBestDistance = tDistance;
                tBest = tCurrent;
            }

            if (tCurrent == tGoal)
            {
                pEnd = tGoal;
                return true;
            }

            const float tCurrentCost = tBuffers.cost[tCurrent];

            for (const auto& tOffset : kNeighbours)
            {
                int tNextX = tX + tOffset[0];
                int tNextZ = tZ + tOffset[1];

                if (!pMap.isCellWalkable(tNextX, tNextZ))
                {
                    continue;
                }

                bool tIsDiagonal = tOffset[0] != 0 && tOffset[1] != 0;

                // No cutting corners: a diagonal step needs both sides open.
                if (tIsDiagonal && (!pMap.isCellWalkable(tX + tOffset[0], tZ) || !pMap.isCellWalkable(tX, tZ + tOffset[1])))
                {
                    continue;
                }

                float tStep = tIsDiagonal ? kDiagonalCost : 1.0f;

                if (pMap.isCellRoad(tNextX, tNextZ))
                {
                    tStep *= kRoadCostFactor;
                }

                int32_t tNext = toIndex(tNextX, tNextZ);
                tBuffers.touch(tNext);

                float tNewCost = tCurrentCost + tStep;

                if (tBuffers.isClosed[tNext] || tNewCost >= tBuffers.cost[tNext])
                {
                    continue;
                }

                tBuffers.cost[tNext] = tNewCost;
                tBuffers.parent[tNext] = tCurrent;

                tOpen.push({ tNewCost + pHeuristicFactor * octile(tGoalX - tNextX, tGoalZ - tNextZ), tNext });
            }
        }

        pEnd = tBest;
        return false;
    };

    // First a search that genuinely prefers roads, on a budget that covers
    // everyday trips. If that is not enough - a walk across the whole map -
    // a goal-directed search still gets there, with less regard for roads.
    int32_t tEnd = tStart;
    bool tIsReached = search(kHeuristicFactor, kRoadAwareExpansions, tEnd)
                      || search(1.0f, kMaxExpansions, tEnd);


    if (tEnd == tStart && !tIsReached)
    {
        return {};
    }

    // Cells back to front, then as points.
    std::vector<glm::vec2> tPoints;

    for (int32_t tCell = tEnd; tCell != -1; tCell = tBuffers.parent[tCell])
    {
        tPoints.emplace_back(pMap.getCellCenterX(tCell % tResolution), pMap.getCellCenterZ(tCell / tResolution));
    }

    std::reverse(tPoints.begin(), tPoints.end());

    tPoints.front() = pFrom;

    if (tIsReached)
    {
        tPoints.back() = tGoalPoint;
    }

    // String pulling: from each anchor, extend the straight leg point by point
    // while it stays walkable. Forward and greedy, so a long winding path
    // costs a few checks per point rather than one per pair of points. A leg
    // the search routed along a road is only straightened along the road, so
    // smoothing does not undo the preference for roads.
    std::vector<glm::vec2> tWaypoints;
    size_t tAnchor = 0;

    while (tAnchor + 1 < tPoints.size())
    {
        size_t tNext = tAnchor + 1;

        auto isOnRoad = [&pMap](const glm::vec2& pPoint)
        {
            return pMap.isCellRoad(pMap.getCellIndex(pPoint.x), pMap.getCellIndex(pPoint.y));
        };

        bool tIsRoadLeg = isOnRoad(tPoints[tAnchor]) && isOnRoad(tPoints[tNext]);

        while (tNext + 1 < tPoints.size())
        {
            const glm::vec2& tCandidate = tPoints[tNext + 1];

            if (tIsRoadLeg && isOnRoad(tCandidate) && !isLegOnRoad(pMap, tPoints[tAnchor], tCandidate))
            {
                break;
            }

            if (!pMap.isPathWalkable(tPoints[tAnchor].x, tPoints[tAnchor].y, tCandidate.x, tCandidate.y, kLegSampleStep))
            {
                break;
            }

            tIsRoadLeg = tIsRoadLeg && isOnRoad(tCandidate);
            ++tNext;
        }

        tWaypoints.push_back(tPoints[tNext]);
        tAnchor = tNext;
    }

    return tWaypoints;
}
