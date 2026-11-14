#include "ParticleBatch.h"

#include <glm/vec2.hpp>

using namespace lakot;

namespace
{
    constexpr const char* kCenterSize = "centerSize";
    constexpr const char* kColor = "color";
    constexpr const char* kVelocityStretch = "velocityStretch";
    constexpr const char* kParams = "params";
}

ParticleBatch::~ParticleBatch()
{
    deinitialize();
}

ParticleBatch::ParticleBatch()
    : Renderable()
{

}

void ParticleBatch::initialize()
{
    if (mIsInitialized)
    {
        return;
    }

    mVertexInformation.set<glm::vec2>("corners",
    {
        glm::vec2(-0.5f, -0.5f),
        glm::vec2( 0.5f, -0.5f),
        glm::vec2( 0.5f,  0.5f),
        glm::vec2(-0.5f,  0.5f)
    });

    mVertexInformation.set<unsigned int>("indices", { 0, 1, 2, 2, 3, 0 });

    createIndexBuffer("indices");
    createStaticBuffer("corners", VertexBufferObjectDataType::eVec2);
    createInstancedBuffer(kCenterSize, VertexBufferObjectDataType::eVec4);
    createInstancedBuffer(kColor, VertexBufferObjectDataType::eVec4);
    createInstancedBuffer(kVelocityStretch, VertexBufferObjectDataType::eVec4);
    createInstancedBuffer(kParams, VertexBufferObjectDataType::eVec4);

    mVertexArrayObject.initialize();

    syncIndexData("indices");
    syncBufferData<glm::vec2>(0, "corners");

    mIsNeedUpdate = false;
    mIsInitialized = true;
}

void ParticleBatch::deinitialize()
{
    if (!mIsInitialized)
    {
        return;
    }

    mVertexArrayObject.deinitialize();

    mIsNeedUpdate = false;
    mIsInitialized = false;
}

void ParticleBatch::setInstances(const std::vector<glm::vec4>& pCenterSize, const std::vector<glm::vec4>& pColor,
                                 const std::vector<glm::vec4>& pVelocityStretch, const std::vector<glm::vec4>& pParams)
{
    mVertexInformation.getBuffer<glm::vec4>(kCenterSize).assign(pCenterSize.begin(), pCenterSize.end());
    mVertexInformation.getBuffer<glm::vec4>(kColor).assign(pColor.begin(), pColor.end());
    mVertexInformation.getBuffer<glm::vec4>(kVelocityStretch).assign(pVelocityStretch.begin(), pVelocityStretch.end());
    mVertexInformation.getBuffer<glm::vec4>(kParams).assign(pParams.begin(), pParams.end());

    if (!mIsInitialized || pCenterSize.empty())
    {
        return;
    }

    syncBufferData<glm::vec4>(1, kCenterSize);
    syncBufferData<glm::vec4>(2, kColor);
    syncBufferData<glm::vec4>(3, kVelocityStretch);
    syncBufferData<glm::vec4>(4, kParams);
}

unsigned int ParticleBatch::getInstanceCount()
{
    return static_cast<unsigned int>(mVertexInformation.getBuffer<glm::vec4>(kCenterSize).size());
}
