#ifndef LAKOT_TERRAIN_MESH_H
#define LAKOT_TERRAIN_MESH_H

#include <vector>

#include <glm/glm.hpp>

namespace lakot
{

struct TerrainChunk
{
    unsigned int indexOffset = 0;
    unsigned int indexCount = 0;
    glm::vec2 center{0.0f};
    float radius = 0.0f;
};

struct TerrainMesh
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;
    std::vector<unsigned int> indices;
    std::vector<TerrainChunk> chunks;
};

}

#endif
