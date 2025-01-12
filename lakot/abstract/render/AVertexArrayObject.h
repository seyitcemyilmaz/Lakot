#ifndef LAKOT_AVERTEXARRAYOBJECT_H
#define LAKOT_AVERTEXARRAYOBJECT_H

#include "lakot/utilities/Object.h"

namespace lakot {

class AVertexArrayObject : public Object
{
public:
    virtual ~AVertexArrayObject() override;
    AVertexArrayObject();

    virtual void bind() const = 0;

    virtual unsigned int getIndiceCount() const = 0;

    virtual unsigned int getId() const;
protected:
    unsigned int mId;
};

}

#endif
