#include "VertexBufferObject.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

using namespace lakot;

VertexBufferObject::~VertexBufferObject()
{

}

VertexBufferObject::VertexBufferObject(const std::string& pName,
                                       VertexBufferObjectBufferType pBufferType,
                                       VertexBufferObjectDataType pDataType,
                                       VertexBufferObjectDrawType pDrawType)
    : mName(pName)
    , mId(UINT_MAX)
    , mLayoutLocation(UINT_MAX)
    , mBufferType(pBufferType)
    , mDrawType(pDrawType)
    , mDataType(pDataType)
    , mDataCount(0)
    , mIsInstanced(false)
{

}

void VertexBufferObject::initialize()
{
    glGenBuffers(1, &mId);
}

void VertexBufferObject::deinitialize()
{
    glDeleteBuffers(1, &mId);
}

void VertexBufferObject::bind()
{
    switch (mBufferType)
    {
        case VertexBufferObjectBufferType::eElementArrayBuffer:
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);
            break;
        }
        case VertexBufferObjectBufferType::eArrayBuffer:
        {
            glBindBuffer(GL_ARRAY_BUFFER, mId);
            break;
        }
        default:
        {
            SDL_Log("Vertex Buffer Object buffer type is unknown type.");
        }
    }
}

const std::string& VertexBufferObject::getName() const
{
    return mName;
}

VertexBufferObjectBufferType VertexBufferObject::getBufferType() const
{
    return mBufferType;
}

VertexBufferObjectDrawType VertexBufferObject::getDrawType() const
{
    return mDrawType;
}

VertexBufferObjectDataType VertexBufferObject::getDataType() const
{
    return mDataType;
}

unsigned int VertexBufferObject::getDataCount() const
{
    return mDataCount;
}

void VertexBufferObject::setLayoutLocation(unsigned int pLayoutLocation)
{
    mLayoutLocation = pLayoutLocation;
}

bool VertexBufferObject::getIsInstanced() const
{
    return mIsInstanced;
}

void VertexBufferObject::setIsInstanced(bool pIsInstanced)
{
    mIsInstanced = pIsInstanced;
}

void VertexBufferObject::setData(const std::vector<unsigned int>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(unsigned int), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::vec2>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::vec2), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::vec3>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::vec3), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::vec4>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::vec4), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::ivec2>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::ivec2), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::ivec3>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::ivec3), pData.data(), tDrawType);
}

void VertexBufferObject::setData(const std::vector<glm::ivec4>& pData)
{
    unsigned int tBufferType = getTypeInternal();

    if (tBufferType == UINT_MAX)
    {
        return;
    }

    unsigned int tDrawType = getDrawTypeInternal();

    if (tDrawType == UINT_MAX)
    {
        return;
    }

    mDataCount = static_cast<unsigned int>(pData.size());

    bind();
    glBufferData(tBufferType, mDataCount * sizeof(glm::ivec4), pData.data(), tDrawType);
}

unsigned int VertexBufferObject::getTypeInternal() const
{
    switch (mBufferType)
    {
        case VertexBufferObjectBufferType::eElementArrayBuffer:
        {
            return GL_ELEMENT_ARRAY_BUFFER;
        }
        case VertexBufferObjectBufferType::eArrayBuffer:
        {
            return GL_ARRAY_BUFFER;
        }
        default:
        {
            SDL_Log("Vertex Buffer Object buffer type is unknown type.");
            break;
        }
    }

    return UINT_MAX;
}

unsigned int VertexBufferObject::getDrawTypeInternal() const
{
    switch (mDrawType)
    {
        case VertexBufferObjectDrawType::eStaticDraw:
        {
            return GL_STATIC_DRAW;
        }
        case VertexBufferObjectDrawType::eDynamicDraw:
        {
            return GL_DYNAMIC_DRAW;
        }
        case VertexBufferObjectDrawType::eStreamDraw:
        {
            return GL_STREAM_DRAW;
        }
        default:
        {
            SDL_Log("Vertex Buffer Object draw type is unknown type.");
            break;
        }
    }

    return UINT_MAX;
}
