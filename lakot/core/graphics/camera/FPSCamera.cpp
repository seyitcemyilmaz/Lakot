#include "FPSCamera.h"

using namespace lakot;

FPSCamera::~FPSCamera()
{

}

FPSCamera::FPSCamera()
    : Camera()
    , mYaw(270.0)
    , mPitch(0.0)
{

}

void FPSCamera::initialize()
{

}

void FPSCamera::deinitialize()
{

}

void FPSCamera::update()
{
    mFrontVector = glm::normalize(glm::vec3(
        cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)),
        sin(glm::radians(mPitch)),
        sin(glm::radians(mYaw)) * cos(glm::radians(mPitch))));
    mRightVector = glm::normalize(glm::cross(mFrontVector, mWorldUpVector));
    mUpVector = glm::normalize(glm::cross(mRightVector, mFrontVector));
}
