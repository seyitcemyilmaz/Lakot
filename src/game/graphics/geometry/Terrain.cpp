#include "Terrain.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

#include <glad/glad.h>

using namespace lakot;

Terrain::~Terrain()
{
    deinitialize();
}

Terrain::Terrain(TerrainMesh pMesh)
    : Renderable()
    , mMesh(std::move(pMesh))
{
    mRenderableType = RenderableType::eTerrain;
}

void Terrain::initialize()
{
    if (!mIsInitialized)
    {
        upload();
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

    for (const TerrainChunk& tChunk : mMesh.chunks)
    {
        if (glm::distance(tCenter, tChunk.center) - tChunk.radius > pMaxDistance)
        {
            continue;
        }

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(tChunk.indexCount), GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(static_cast<uintptr_t>(tChunk.indexOffset) * sizeof(unsigned int)));
    }
}

void Terrain::upload()
{
    mVertexInformation.set("positions", std::move(mMesh.positions));
    mVertexInformation.set("normals", std::move(mMesh.normals));
    mVertexInformation.set("colors", std::move(mMesh.colors));
    mVertexInformation.set("indices", std::move(mMesh.indices));

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
