#include "BoundingBox.h"

using namespace lakot;

BoundingBox::~BoundingBox()
{

}

BoundingBox::BoundingBox(const glm::vec3& pMin, const glm::vec3& pMax)
    : mMin(pMin)
    , mMax(pMax)
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
