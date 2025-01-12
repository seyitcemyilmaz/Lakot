#include "AVertexArrayObject.h"

#include <limits.h>

using namespace lakot;

AVertexArrayObject::~AVertexArrayObject()
{

}

AVertexArrayObject::AVertexArrayObject()
    : Object()
    , mId(UINT_MAX)
{

}

unsigned int AVertexArrayObject::getId() const
{
    return mId;
}
