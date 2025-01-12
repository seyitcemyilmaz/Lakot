#include "AMesh.h"

using namespace lakot;

AMesh::~AMesh()
{

}

AMesh::AMesh(const BoundingBox& pBoundingBox)
    : Object()
    , mBoundingBox(pBoundingBox)
{

}

BoundingBox& AMesh::getBoundingBox()
{
    return mBoundingBox;
}
