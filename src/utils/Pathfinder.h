#ifndef LAKOT_PATHFINDER_H
#define LAKOT_PATHFINDER_H

#include <vector>

#include <glm/glm.hpp>

#include "MapData.h"

namespace lakot
{

// A* over a map's walk grid (the block map's cells). Shared by the client
// (click to move) and - later - the server (monsters and NPCs).
//
// Never fails outright: an unwalkable destination is swapped for the nearest
// walkable cell, and an unreachable one for the closest point the search
// could get to. The result is smoothed into straight legs, each of which is
// walkable end to end, so following it never trips the server's movement
// check.
class Pathfinder
{
public:
    // Upper bound on expanded cells, so one click can never stall a frame for
    // long. Roughly a search across the whole of a 2048-unit map.
    static constexpr int kMaxExpansions = 250000;

    // Waypoints after pFrom, ending at the destination actually reached.
    // Empty if there is nowhere to go (already there, or boxed in).
    static std::vector<glm::vec2> findPath(const MapData& pMap, const glm::vec2& pFrom, const glm::vec2& pTo);

private:
    Pathfinder() = default;
    ~Pathfinder() = default;
};

}

#endif
