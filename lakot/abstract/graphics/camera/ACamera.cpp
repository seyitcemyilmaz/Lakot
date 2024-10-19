#include "ACamera.h"

using namespace lakot;

ACamera::~ACamera()
{

}

ACamera::ACamera()
    : Object()
    , mType(CameraType::eUndefined)
    , mPosition(0.0f, 0.0f, 0.0f)
    , mFrontVector(0.0f, 0.0f, -1.0f)
    , mUpVector(0.0f, 1.0f, 0.0f)
    , mRightVector(1.0f, 0.0f, 0.0f)
    , mWorldUpVector(0.0f, 1.0f, 0.0f)
{

}

CameraType ACamera::getType() const
{
    return mType;
}

const glm::vec3& ACamera::getPosition() const
{
    return mPosition;
}

void ACamera::setPosition(const glm::vec3& pPosition)
{
    mPosition = pPosition;
}

const glm::vec3& ACamera::getFrontVector() const
{
    return mFrontVector;
}

void ACamera::setFrontVector(const glm::vec3& pFrontVector)
{
    mFrontVector = pFrontVector;
}

const glm::vec3& ACamera::getUpVector() const
{
    return mUpVector;
}

void ACamera::setUpVector(const glm::vec3& pUpVector)
{
    mUpVector = pUpVector;
}

const glm::vec3& ACamera::getRightVector() const
{
    return mRightVector;
}

void ACamera::setRightVector(const glm::vec3& pRightVector)
{
    mRightVector = pRightVector;
}

const glm::vec3& ACamera::getWorldUpVector() const
{
    return mWorldUpVector;
}

void ACamera::setWorldUpVector(const glm::vec3& pWorldUpVector)
{
    mWorldUpVector = pWorldUpVector;
}

void ACamera::changePosition(const glm::vec3& pAmount)
{
    mPosition += pAmount;
}

const glm::mat4& ACamera::getViewMatrix() const
{
    return mViewMatrix;
}
