#include "ARenderable.h"

using namespace lakot;

ARenderable::~ARenderable()
{

}

ARenderable::ARenderable()
    : IRenderable()
    , mType(RenderableType::eUnknown)
{

}

RenderableType ARenderable::getType() const
{
    return mType;
}
