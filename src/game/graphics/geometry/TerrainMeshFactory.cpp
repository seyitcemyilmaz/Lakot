#include "TerrainMeshFactory.h"

#include <algorithm>
#include <cmath>

using namespace lakot;

TerrainMesh TerrainMeshFactory::create(const MapData& pMap)
{
    TerrainMesh tMesh;

    const int tResolution = pMap.getResolution();
    const float tSpacing = pMap.getGridSpacing();

    std::vector<glm::vec3>& tPositions = tMesh.positions;
    std::vector<glm::vec3>& tNormals = tMesh.normals;
    std::vector<glm::vec3>& tColors = tMesh.colors;

    tPositions.reserve(static_cast<size_t>(tResolution) * tResolution);
    tNormals.reserve(tPositions.capacity());
    tColors.reserve(tPositions.capacity());

    for (int tZ = 0; tZ < tResolution; ++tZ)
    {
        for (int tX = 0; tX < tResolution; ++tX)
        {
            tPositions.emplace_back(pMap.getVertexX(tX), pMap.getVertexHeight(tX, tZ), pMap.getVertexZ(tZ));

            float tLeft = pMap.getVertexHeight(std::max(tX - 1, 0), tZ);
            float tRight = pMap.getVertexHeight(std::min(tX + 1, tResolution - 1), tZ);
            float tUp = pMap.getVertexHeight(tX, std::max(tZ - 1, 0));
            float tDown = pMap.getVertexHeight(tX, std::min(tZ + 1, tResolution - 1));
            tNormals.push_back(glm::normalize(glm::vec3(tLeft - tRight, 2.0f * tSpacing, tUp - tDown)));

            auto tColor = pMap.getVertexColor(tX, tZ);
            tColors.emplace_back(tColor[0] / 255.0f, tColor[1] / 255.0f, tColor[2] / 255.0f);
        }
    }

    std::vector<unsigned int>& tIndices = tMesh.indices;
    tIndices.reserve(static_cast<size_t>(tResolution - 1) * (tResolution - 1) * 6);

    const int tCells = tResolution - 1;

    for (int tChunkZ = 0; tChunkZ < tCells; tChunkZ += kChunkCells)
    {
        for (int tChunkX = 0; tChunkX < tCells; tChunkX += kChunkCells)
        {
            TerrainChunk tChunk;
            tChunk.indexOffset = static_cast<unsigned int>(tIndices.size());

            int tEndX = std::min(tChunkX + kChunkCells, tCells);
            int tEndZ = std::min(tChunkZ + kChunkCells, tCells);

            for (int tZ = tChunkZ; tZ < tEndZ; ++tZ)
            {
                for (int tX = tChunkX; tX < tEndX; ++tX)
                {
                    unsigned int tTopLeft = static_cast<unsigned int>(tZ * tResolution + tX);
                    unsigned int tTopRight = tTopLeft + 1;
                    unsigned int tBottomLeft = static_cast<unsigned int>((tZ + 1) * tResolution + tX);
                    unsigned int tBottomRight = tBottomLeft + 1;

                    tIndices.push_back(tTopLeft);
                    tIndices.push_back(tBottomLeft);
                    tIndices.push_back(tTopRight);

                    tIndices.push_back(tTopRight);
                    tIndices.push_back(tBottomLeft);
                    tIndices.push_back(tBottomRight);
                }
            }

            tChunk.indexCount = static_cast<unsigned int>(tIndices.size()) - tChunk.indexOffset;

            float tMinX = pMap.getVertexX(tChunkX);
            float tMaxX = pMap.getVertexX(tEndX);
            float tMinZ = pMap.getVertexZ(tChunkZ);
            float tMaxZ = pMap.getVertexZ(tEndZ);

            tChunk.center = glm::vec2((tMinX + tMaxX) * 0.5f, (tMinZ + tMaxZ) * 0.5f);
            tChunk.radius = glm::length(glm::vec2(tMaxX - tMinX, tMaxZ - tMinZ)) * 0.5f;

            tMesh.chunks.push_back(tChunk);
        }
    }

    return tMesh;
}
