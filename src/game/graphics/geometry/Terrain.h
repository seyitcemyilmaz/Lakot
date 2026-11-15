#ifndef LAKOT_TERRAIN_H
#define LAKOT_TERRAIN_H

#include <vector>

#include <glm/glm.hpp>

#include "TerrainMesh.h"

#include "../render/Renderable.h"

namespace lakot
{

class Terrain final : public Renderable
{
public:
    ~Terrain() override;
    explicit Terrain(TerrainMesh pMesh);

    void initialize() override;
    void deinitialize() override;

    // Issues one draw per chunk within pMaxDistance of pCenter (horizontal).
    // The terrain's VAO must already be bound.
    void drawChunksNear(const glm::vec3& pCenter, float pMaxDistance) const;

private:
    TerrainMesh mMesh;

    void upload();
};

}

#endif
