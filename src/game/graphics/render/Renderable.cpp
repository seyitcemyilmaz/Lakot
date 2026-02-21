#include "Renderable.h"

using namespace lakot;

Renderable::~Renderable()
{

}

Renderable::Renderable()
    : mRenderableType(RenderableType::eUndefined)
    , mIsNeedUpdate(false)
    , mIsInitialized(false)
{

}

const VertexArrayObject& Renderable::getVertexArrayObject() const
{
    return mVertexArrayObject;
}

void Renderable::createIndexBuffer(const std::string& pDataKey)
{
    VertexBufferObject* tBuffer = new VertexBufferObject(
        pDataKey,
        VertexBufferObjectBufferType::eElementArrayBuffer,
        VertexBufferObjectDataType::eUnsignedInt,
        VertexBufferObjectDrawType::eStaticDraw
    );

    bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

    if (!tIsAdded)
    {
        throw tBuffer->getName() + " is not added.";
    }
}

void Renderable::createStaticBuffer(const std::string& pDataKey, VertexBufferObjectDataType pDataType)
{
    VertexBufferObject* tBuffer = new VertexBufferObject(
        pDataKey,
        VertexBufferObjectBufferType::eArrayBuffer,
        pDataType,
        VertexBufferObjectDrawType::eStaticDraw
    );

    bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

    if (!tIsAdded)
    {
        throw tBuffer->getName() + " is not added.";
    }
}

void Renderable::createInstancedBuffer(const std::string& pDataKey, VertexBufferObjectDataType pDataType)
{
    VertexBufferObject* tBuffer = new VertexBufferObject(
        pDataKey,
        VertexBufferObjectBufferType::eArrayBuffer,
        pDataType,
        VertexBufferObjectDrawType::eDynamicDraw
    );

    tBuffer->setIsInstanced(true);

    bool tIsAdded = mVertexArrayObject.addVertexBufferObject(tBuffer);

    if (!tIsAdded)
    {
        throw tBuffer->getName() + " is not added.";
    }
}

void Renderable::syncIndexData(const std::string& pDataKey)
{
    VertexBufferObject* tBuffer = mVertexArrayObject.getElementBufferObject();

    if (tBuffer)
    {
        if (auto tIndices = mVertexInformation.get<unsigned int>(pDataKey))
        {
            tBuffer->setData(tIndices->get());
        }
        else
        {
            // TODO: add message
        }
    }
    else
    {
        // TODO: add message
    }
}
