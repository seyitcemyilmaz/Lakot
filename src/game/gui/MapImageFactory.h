#ifndef LAKOT_MAP_IMAGE_FACTORY_H
#define LAKOT_MAP_IMAGE_FACTORY_H

#include <cstdint>
#include <vector>

#include "MapData.h"

namespace lakot
{

struct MapImage
{
    int size = 0;                  // square, size x size pixels
    std::vector<uint8_t> rgba;     // row 0 is the north edge (-z)
};

// Paints a top-down picture of a map from its MapData - ground colour with
// hill shading, water, and object footprints - for the minimap and the world
// map. Made from the same data the terrain is built from, so it can never go
// out of date against it.
class MapImageFactory
{
public:
    static MapImage create(const MapData& pMap, int pSize);

private:
    MapImageFactory() = default;
    ~MapImageFactory() = default;
};

}

#endif
