#ifndef LAKOT_VERTEXARRAYOBJECT_H
#define LAKOT_VERTEXARRAYOBJECT_H

#include <lakot/abstract/render/AVertexArrayObject.h>

#include "VertexBufferObject.h"

namespace lakot {

class VertexArrayObject final : public AVertexArrayObject
{
public:
    virtual ~VertexArrayObject() override;
    VertexArrayObject();

    void initialize() override;
    void deinitialize() override;

    void bind() const override;

    unsigned int getIndiceCount() const override;

    // bool getIsBufferExist(const std::string& pBufferName) const;

    bool addVertexBufferObject(VertexBufferObject* pVertexBufferObject);

    unsigned int getVertexBufferObjectCount() const;
    VertexBufferObject* getVertexBufferObject(unsigned int pIndex) const;

    VertexBufferObject* getElementBufferObject() const;

    // VertexBufferObject* operator[](const std::string& pBufferName);

private:
    // std::map<std::string, VertexBufferObject*> mBuffers;

    std::vector<VertexBufferObject*> mBuffers;
    VertexBufferObject* mElementBufferObject;

    void generate();
};

}

#endif
