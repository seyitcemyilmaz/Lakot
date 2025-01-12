#include "ABone.h"

using namespace lakot;

ABone::~ABone()
{

}

ABone::ABone(const glm::mat4& pTransformation)
    : Object()
    , mTransformation(pTransformation)
{

}

glm::mat4& ABone::getTransformation()
{
    return mTransformation;
}
