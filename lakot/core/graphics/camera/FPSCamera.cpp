#include "FPSCamera.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

using namespace lakot;

FPSCamera::~FPSCamera()
{

}

FPSCamera::FPSCamera()
    : Camera()
    , mYaw(270.0)
    , mPitch(0.0)
{
    mType = CameraType::eFPS;
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

    mViewMatrix = glm::lookAt(mPosition, mPosition + mFrontVector, mUpVector);
}
