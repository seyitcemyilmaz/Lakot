#include "VertexArrayObject.h"

#if defined(LAKOT_RENDERER_OPENGL)
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
#include <GL/glew.h>
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#else
#error "Undefined OpenGL Type."
#endif
#endif

#include <spdlog/spdlog.h>

using namespace lakot;

VertexArrayObject::~VertexArrayObject()
{

}

VertexArrayObject::VertexArrayObject()
    : AVertexArrayObject()
    , mElementBufferObject(nullptr)
{

}

void VertexArrayObject::initialize()
{
    if (!mElementBufferObject)
    {
        spdlog::error("Vertex Array Object has no element buffer.");
        return;
    }

    if (mBuffers.size() == 0)
    {
        spdlog::error("Vertex Array Object has no array buffer.");
        return;
    }

    generate();
    bind();

    mElementBufferObject->initialize();

    unsigned int tBufferCount = static_cast<unsigned int>(mBuffers.size());

    for (unsigned int i = 0; i < tBufferCount; i++)
    {
        VertexBufferObject* tBuffer = mBuffers[i];

        if (!tBuffer)
        {
            continue;
        }

        VertexBufferObjectDataType tDataType = tBuffer->getDataType();

        unsigned int tSize = 0;
        bool tIsInt = false;

        switch (tDataType)
        {
            case VertexBufferObjectDataType::eVec2:
            {
                tSize = 2;
                break;
            }
            case VertexBufferObjectDataType::eVec3:
            {
                tSize = 3;
                break;
            }
            case VertexBufferObjectDataType::eVec4:
            {
                tSize = 4;
                break;
            }
            case VertexBufferObjectDataType::eIVec2:
            {
                tSize = 2;
                tIsInt = true;
                break;
            }
            case VertexBufferObjectDataType::eIVec3:
            {
                tSize = 3;
                tIsInt = true;
                break;
            }
            case VertexBufferObjectDataType::eIVec4:
            {
                tSize = 4;
                tIsInt = true;
                break;
            }
            case VertexBufferObjectDataType::eUnsignedInt:
            {
                tSize = 1;
                tIsInt = true;
                break;
            }
            default:
            {
                throw "Vertex Buffer Object undefined data type.";
            }
        }

        tBuffer->initialize();
        tBuffer->bind();

        if (!tIsInt)
        {
            glVertexAttribPointer(i, tSize, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(i);
        }
        else
        {
            glVertexAttribIPointer(i, tSize, GL_INT, 0, (void*)0);
            glEnableVertexAttribArray(i);
        }

        if (tBuffer->getIsInstanced())
        {
            glVertexAttribDivisor(i, 1);
        }
    }
}

void VertexArrayObject::deinitialize()
{
    bind();

    glDeleteVertexArrays(1, &mId);

    for (auto& tElement : mBuffers)
    {
        VertexBufferObject* pBuffer = tElement;

        if (!pBuffer)
        {
            continue;
        }

        pBuffer->bind();
        pBuffer->deinitialize();
        delete pBuffer;
    }

    mBuffers.clear();
}

void VertexArrayObject::bind() const
{
    glBindVertexArray(mId);
}

unsigned int VertexArrayObject::getIndiceCount() const
{
    if (!mElementBufferObject)
    {
        return 0;
    }

    return mElementBufferObject->getDataCount();
}

bool VertexArrayObject::addVertexBufferObject(VertexBufferObject* pVertexBufferObject)
{
    if (!pVertexBufferObject)
    {
        spdlog::error("Vertex Buffer Object is nullptr.");
        return false;
    }

    VertexBufferObjectBufferType tBufferType = pVertexBufferObject->getBufferType();

    switch (tBufferType)
    {
        case VertexBufferObjectBufferType::eElementArrayBuffer:
        {
            if (mElementBufferObject == nullptr)
            {
                mElementBufferObject = pVertexBufferObject;
                return true;
            }
            else
            {
                spdlog::error("Element buffer object is already added.");
            }

            break;
        }
        case VertexBufferObjectBufferType::eArrayBuffer:
        {
            mBuffers.push_back(pVertexBufferObject);
            return true;
        }
        default:
        {
            spdlog::error("Vertex Buffer Object buffer type is unknown type.");
        }
    }

    return false;
}

unsigned int VertexArrayObject::getVertexBufferObjectCount() const
{
    return static_cast<unsigned int>(mBuffers.size());
}

VertexBufferObject* VertexArrayObject::getVertexBufferObject(unsigned int pIndex) const
{
    if (pIndex >= getVertexBufferObjectCount() || pIndex < 0)
    {
        spdlog::error("Vertex Array Object wrong index.");
        return nullptr;
    }

    return mBuffers[pIndex];
}

VertexBufferObject* VertexArrayObject::getElementBufferObject() const
{
    return mElementBufferObject;
}

void VertexArrayObject::generate()
{
    glGenVertexArrays(1, &mId);
}

// bool VertexArrayObject::getIsBufferExist(const std::string& pBufferName) const
// {
//     auto tIterator = mBuffers.find(pBufferName);

//     return tIterator != mBuffers.end();
// }

// VertexBufferObject* VertexArrayObject::operator[](const std::string& pBufferName)
// {
//     VertexBufferObject* tVertexBufferObject = nullptr;

//     auto tIterator = mBuffers.find(pBufferName);

//     if (tIterator != mBuffers.end())
//     {
//         tVertexBufferObject = tIterator->second;
//     }
//     else
//     {
//         tVertexBufferObject = new VertexBufferObject();
//     }

//     return tVertexBufferObject;
// }


