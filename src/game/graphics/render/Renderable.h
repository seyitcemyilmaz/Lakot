#ifndef LAKOT_RENDERABLE_H
#define LAKOT_RENDERABLE_H

#include "Type.h"
#include "DataContainer.hpp"

#include "VertexArrayObject.h"

namespace lakot
{

class Renderable
{
public:
    virtual ~Renderable();
    explicit Renderable();

    virtual void initialize() = 0;
    virtual void deinitialize() = 0;

    const VertexArrayObject& getVertexArrayObject() const;

protected:
    DataContainer mVertexInformation;
    VertexArrayObject mVertexArrayObject;

    RenderableType mRenderableType;

    bool mIsNeedUpdate;
    bool mIsInitialized;

    void createIndexBuffer(const std::string& pDataKey);
    void createStaticBuffer(const std::string& pDataKey, VertexBufferObjectDataType pDataType);
    void createInstancedBuffer(const std::string& pDataKey, VertexBufferObjectDataType pDataType);

    void syncIndexData(const std::string& pDataKey);

    template <typename T>
    void syncBufferData(int pLayoutLocation, const std::string& pDataKey)
    {
        VertexBufferObject* tBuffer = mVertexArrayObject.getVertexBufferObject(pLayoutLocation);

        if (tBuffer)
        {
            const std::vector<T>& tData = mVertexInformation.getBuffer<T>(pDataKey);
            tBuffer->setData(tData);
        }
        else
        {
            // TODO: add message.
        }
    }
};

}

#endif
