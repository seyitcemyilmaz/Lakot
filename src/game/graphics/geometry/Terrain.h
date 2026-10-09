#ifndef LAKOT_TERRAIN_H
#define LAKOT_TERRAIN_H

#include <vector>

#include <glm/glm.hpp>

#include "MapData.h"

#include "../render/Renderable.h"

namespace lakot
{

// The ground of one map, built from its MapData: positions, smooth normals
// and ground colour per vertex. The mesh is split into square chunks sharing
// one vertex buffer, so only the chunks near the camera are drawn.
class Terrain final : public Renderable
{
public:
    ~Terrain() override;
    explicit Terrain(const MapData& pMap);

    void initialize() override;
    void deinitialize() override;

    // Issues one draw per chunk within pMaxDistance of pCenter (horizontal).
    // The terrain's VAO must already be bound.
    void drawChunksNear(const glm::vec3& pCenter, float pMaxDistance) const;

private:
    struct Chunk
    {
        unsigned int indexOffset = 0;
        unsigned int indexCount = 0;
        glm::vec2 center{0.0f};
        float radius = 0.0f;
    };

    static constexpr int kChunkCells = 32;

    const MapData& mMap;
    std::vector<Chunk> mChunks;

    void generateMesh();
};

}

#endif
