#include "BoundingBox.h"

using namespace lakot;

BoundingBox::~BoundingBox()
{

}

BoundingBox::BoundingBox(const glm::vec3& mMin, const glm::vec3& mMax)
    : Object()
    , mMin(mMin)
    , mMax(mMax)
{

}

void BoundingBox::initialize()
{

}

void BoundingBox::deinitialize()
{

}

const glm::vec3& BoundingBox::getMin() const
{
    return mMin;
}

void BoundingBox::setMin(const glm::vec3& pMin)
{
    mMin = pMin;
}

const glm::vec3& BoundingBox::getMax() const
{
    return mMax;
}

void BoundingBox::setMax(const glm::vec3& pMax)
{
    mMax = pMax;
}
