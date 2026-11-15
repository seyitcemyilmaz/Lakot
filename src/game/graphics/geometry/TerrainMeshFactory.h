#ifndef LAKOT_TERRAIN_MESH_FACTORY_H
#define LAKOT_TERRAIN_MESH_FACTORY_H

#include "MapData.h"

#include "TerrainMesh.h"

namespace lakot
{

class TerrainMeshFactory
{
public:
    static TerrainMesh create(const MapData& pMap);

private:
    static constexpr int kChunkCells = 32;

    TerrainMeshFactory() = default;
    ~TerrainMeshFactory() = default;
};

}

#endif
