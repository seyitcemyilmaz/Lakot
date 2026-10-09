#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glad/glad.h>

using namespace lakot;

Terrain::~Terrain()
{
    deinitialize();
}

Terrain::Terrain(const MapData& pMap)
    : Renderable()
    , mMap(pMap)
{
    mRenderableType = RenderableType::eTerrain;
}

void Terrain::initialize()
{
    if (!mIsInitialized)
    {
        generateMesh();
        mIsInitialized = true;
    }
}

void Terrain::deinitialize()
{
    if (mIsInitialized)
    {
        mVertexArrayObject.deinitialize();
        mIsInitialized = false;
    }
}

void Terrain::drawChunksNear(const glm::vec3& pCenter, float pMaxDistance) const
{
    glm::vec2 tCenter(pCenter.x, pCenter.z);

    for (const Chunk& tChunk : mChunks)
    {
        if (glm::distance(tCenter, tChunk.center) - tChunk.radius > pMaxDistance)
        {
            continue;
        }

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tChunk.indexCount), GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(tChunk.indexOffset) * sizeof(unsigned int)));
    }
}

void Terrain::generateMesh()
{
    const int tResolution = mMap.getResolution();
    const float tSpacing = mMap.getGridSpacing();

    std::vector<glm::vec3> tPositions;
    std::vector<glm::vec3> tNormals;
    std::vector<glm::vec3> tColors;

    tPositions.reserve(static_cast<size_t>(tResolution) * tResolution);
    tNormals.reserve(tPositions.capacity());
    tColors.reserve(tPositions.capacity());

    for (int z = 0; z < tResolution; ++z)
    {
        for (int x = 0; x < tResolution; ++x)
        {
            tPositions.emplace_back(mMap.getVertexX(x), mMap.getVertexHeight(x, z), mMap.getVertexZ(z));

            // Central differences - smooth shading across the grid.
            float tLeft = mMap.getVertexHeight(std::max(x - 1, 0), z);
            float tRight = mMap.getVertexHeight(std::min(x + 1, tResolution - 1), z);
            float tUp = mMap.getVertexHeight(x, std::max(z - 1, 0));
            float tDown = mMap.getVertexHeight(x, std::min(z + 1, tResolution - 1));
            tNormals.push_back(glm::normalize(glm::vec3(tLeft - tRight, 2.0f * tSpacing, tUp - tDown)));

            auto tColor = mMap.getVertexColor(x, z);
            tColors.emplace_back(tColor[0] / 255.0f, tColor[1] / 255.0f, tColor[2] / 255.0f);
        }
    }

    // Indices grouped chunk by chunk, so each chunk is one contiguous range.
    std::vector<unsigned int> tIndices;
    tIndices.reserve(static_cast<size_t>(tResolution - 1) * (tResolution - 1) * 6);

    const int tCells = tResolution - 1;

    for (int tChunkZ = 0; tChunkZ < tCells; tChunkZ += kChunkCells)
    {
        for (int tChunkX = 0; tChunkX < tCells; tChunkX += kChunkCells)
        {
            Chunk tChunk;
            tChunk.indexOffset = static_cast<unsigned int>(tIndices.size());

            int tEndX = std::min(tChunkX + kChunkCells, tCells);
            int tEndZ = std::min(tChunkZ + kChunkCells, tCells);

            for (int z = tChunkZ; z < tEndZ; ++z)
            {
                for (int x = tChunkX; x < tEndX; ++x)
                {
                    unsigned int tTopLeft = static_cast<unsigned int>(z * tResolution + x);
                    unsigned int tTopRight = tTopLeft + 1;
                    unsigned int tBottomLeft = static_cast<unsigned int>((z + 1) * tResolution + x);
                    unsigned int tBottomRight = tBottomLeft + 1;

                    // Diagonal top-right to bottom-left - MapData::getHeightAt
                    // interpolates across the same split.
                    tIndices.push_back(tTopLeft);
                    tIndices.push_back(tBottomLeft);
                    tIndices.push_back(tTopRight);

                    tIndices.push_back(tTopRight);
                    tIndices.push_back(tBottomLeft);
                    tIndices.push_back(tBottomRight);
                }
            }

            tChunk.indexCount = static_cast<unsigned int>(tIndices.size()) - tChunk.indexOffset;

            float tMinX = mMap.getVertexX(tChunkX);
            float tMaxX = mMap.getVertexX(tEndX);
            float tMinZ = mMap.getVertexZ(tChunkZ);
            float tMaxZ = mMap.getVertexZ(tEndZ);

            tChunk.center = glm::vec2((tMinX + tMaxX) * 0.5f, (tMinZ + tMaxZ) * 0.5f);
            tChunk.radius = glm::length(glm::vec2(tMaxX - tMinX, tMaxZ - tMinZ)) * 0.5f;

            mChunks.push_back(tChunk);
        }
    }

    mVertexInformation.set("positions", tPositions);
    mVertexInformation.set("normals", tNormals);
    mVertexInformation.set("colors", tColors);
    mVertexInformation.set("indices", tIndices);

    createIndexBuffer("indices");
    createStaticBuffer("positions", VertexBufferObjectDataType::eVec3);
    createStaticBuffer("normals", VertexBufferObjectDataType::eVec3);
    createStaticBuffer("colors", VertexBufferObjectDataType::eVec3);

    mVertexArrayObject.initialize();

    syncIndexData("indices");
    syncBufferData<glm::vec3>(0, "positions");
    syncBufferData<glm::vec3>(1, "normals");
    syncBufferData<glm::vec3>(2, "colors");

    mIsNeedUpdate = false;
}
