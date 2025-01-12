#include "ARenderable.h"

using namespace lakot;

ARenderable::~ARenderable()
{

}

ARenderable::ARenderable()
    : Object()
    , mRenderableType(RenderableType::eUndefined)
{

}

RenderableType ARenderable::getRenderableType() const
{
    return mRenderableType;
}

VertexInformation& ARenderable::getVertexInformation()
{
    return mVertexInformation;
}
