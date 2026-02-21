#ifndef LAKOT_VERTEXARRAYOBJECT_H
#define LAKOT_VERTEXARRAYOBJECT_H

#include "VertexBufferObject.h"

namespace lakot
{

class VertexArrayObject
{
public:
    virtual ~VertexArrayObject();
    VertexArrayObject();

    void initialize();
    void deinitialize();

    void bind() const;

    unsigned int getIndiceCount() const;

    bool addVertexBufferObject(VertexBufferObject* pVertexBufferObject);

    unsigned int getVertexBufferObjectCount() const;
    VertexBufferObject* getVertexBufferObject(unsigned int pIndex) const;

    VertexBufferObject* getElementBufferObject() const;

private:
    unsigned int mId;

    std::vector<VertexBufferObject*> mBuffers;
    VertexBufferObject* mElementBufferObject;

    void generate();
};

}

#endif
