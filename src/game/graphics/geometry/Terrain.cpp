#include "Terrain.h"

using namespace lakot;

Terrain::~Terrain()
{
    deinitialize();
}

Terrain::Terrain(int pWidth, int pDepth, float pGridSpacing)
    : Renderable()
    , mWidth(pWidth)
    , mDepth(pDepth)
    , mGridSpacing(pGridSpacing)
    , mIndexCount(0)
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

int Terrain::getIndexCount() const
{
    return mIndexCount;
}

void Terrain::generateMesh()
{
    std::vector<glm::vec3> tPositions;
    std::vector<unsigned int> tIndices;

    float tOffsetX = (mWidth * mGridSpacing) / 2.0f;
    float tOffsetZ = (mDepth * mGridSpacing) / 2.0f;

    for (int z = 0; z < mDepth; ++z)
    {
        for (int x = 0; x < mWidth; ++x)
        {
            float tPosX = (x * mGridSpacing) - tOffsetX;
            float tPosZ = (z * mGridSpacing) - tOffsetZ;
            float tPosY = (std::sin(tPosX * 0.2f) + std::cos(tPosZ * 0.2f)) * 2.0f;

            tPositions.push_back(glm::vec3(tPosX, tPosY, tPosZ));
        }
    }

    for (int z = 0; z < mDepth - 1; ++z)
    {
        for (int x = 0; x < mWidth - 1; ++x)
        {
            int tTopLeft = (z * mWidth) + x;
            int tTopRight = tTopLeft + 1;
            int tBottomLeft = ((z + 1) * mWidth) + x;
            int tBottomRight = tBottomLeft + 1;

            tIndices.push_back(tTopLeft);
            tIndices.push_back(tBottomLeft);
            tIndices.push_back(tTopRight);

            tIndices.push_back(tTopRight);
            tIndices.push_back(tBottomLeft);
            tIndices.push_back(tBottomRight);
        }
    }

    mIndexCount = static_cast<int>(tIndices.size());

    mVertexInformation.set("positions", tPositions);
    mVertexInformation.set("indices", tIndices);

    createIndexBuffer("indices");
    createStaticBuffer("positions", VertexBufferObjectDataType::eVec3);

    mVertexArrayObject.initialize();

    syncIndexData("indices");
    syncBufferData<glm::vec3>(0, "positions");

    mIsNeedUpdate = false;
    mIsInitialized = true;
}
