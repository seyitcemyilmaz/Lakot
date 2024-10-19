#include "ARenderable.h"

using namespace lakot;

ARenderable::~ARenderable()
{

}

ARenderable::ARenderable()
    : Object()
    , mType(RenderableType::eUnknown)
{

}

RenderableType ARenderable::getType() const
{
    return mType;
}
