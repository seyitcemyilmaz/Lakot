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
    mCapacityBytes = 0;
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
    upload(pData.data(), pData.size(), sizeof(unsigned int));
}

void VertexBufferObject::setData(const std::vector<glm::vec2>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::vec2));
}

void VertexBufferObject::setData(const std::vector<glm::vec3>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::vec3));
}

void VertexBufferObject::setData(const std::vector<glm::vec4>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::vec4));
}

void VertexBufferObject::setData(const std::vector<glm::ivec2>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::ivec2));
}

void VertexBufferObject::setData(const std::vector<glm::ivec3>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::ivec3));
}

void VertexBufferObject::setData(const std::vector<glm::ivec4>& pData)
{
    upload(pData.data(), pData.size(), sizeof(glm::ivec4));
}

void VertexBufferObject::upload(const void* pData, size_t pCount, size_t pElementSize)
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

    mDataCount = static_cast<unsigned int>(pCount);

    size_t tByteSize = pCount * pElementSize;

    bind();

    // Reuses the existing storage when the data still fits, so a per-frame
    // update of a few instances does not reallocate the buffer every time.
    if (tByteSize <= mCapacityBytes)
    {
        if (tByteSize > 0)
        {
            glBufferSubData(tBufferType, 0, static_cast<GLsizeiptr>(tByteSize), pData);
        }

        return;
    }

    glBufferData(tBufferType, static_cast<GLsizeiptr>(tByteSize), pData, tDrawType);
    mCapacityBytes = tByteSize;
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
