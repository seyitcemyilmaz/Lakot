#include "BoxContainer.h"

using namespace lakot;

BoxContainer::~BoxContainer()
{
    deinitialize();
}

BoxContainer::BoxContainer()
    : Renderable()
{
    mRenderableType = RenderableType::eBoxContainer;
}

void BoxContainer::initialize()
{
    if (mIsInitialized)
    {
        return;
    }

    mVertexInformation.set<glm::vec3>("positions",
    {
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f)
    });

    mVertexInformation.set<unsigned int>("indices",
    {
        0, 1, 2,   2, 3, 0,
        0, 3, 7,   7, 4, 0,
        4, 7, 6,   6, 5, 4,
        1, 5, 6,   6, 2, 1,
        4, 5, 1,   1, 0, 4,
        3, 2, 6,   6, 7, 3
    });

    createIndexBuffer("indices");

    createStaticBuffer("positions", VertexBufferObjectDataType::eVec3);

    createInstancedBuffer("dynamicPosition", VertexBufferObjectDataType::eVec3);

    createInstancedBuffer("dynamicSize", VertexBufferObjectDataType::eVec3);

    mVertexArrayObject.initialize();

    syncIndexData("indices");

    syncBufferData<glm::vec3>(0, "positions");

    syncBufferData<glm::vec3>(1, "dynamicPosition");
    syncBufferData<glm::vec3>(2, "dynamicSize");

    mIsNeedUpdate = false;
    mIsInitialized = true;
}

void BoxContainer::deinitialize()
{
    if (!mIsInitialized)
    {
        return;
    }

    mVertexArrayObject.deinitialize();

    mIsNeedUpdate = false;
    mIsInitialized = false;
}

void BoxContainer::addBox(const glm::vec3& pPosition, const glm::vec3& pSize)
{
    mVertexInformation.append<glm::vec3>("dynamicPosition", pPosition);
    mVertexInformation.append<glm::vec3>("dynamicSize", pSize);

    mIsNeedUpdate = true;

    if (!mIsInitialized)
    {
        return;
    }

    syncBufferData<glm::vec3>(1, "dynamicPosition");
    syncBufferData<glm::vec3>(2, "dynamicSize");

    mIsNeedUpdate = false;
}

unsigned int BoxContainer::getInstanceCount()
{
    auto& tPositions = mVertexInformation.getBuffer<glm::vec3>("dynamicPosition");
    return static_cast<unsigned int>(tPositions.size());
}
