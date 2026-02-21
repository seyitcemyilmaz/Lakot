#include "FPSCamera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace lakot;

FPSCamera::~FPSCamera()
{

}

FPSCamera::FPSCamera()
    : Camera(CameraType::eFPS)
    , mYaw(270.0)
    , mPitch(0.0)
{
    update();
}

void FPSCamera::update()
{
    if (mPitch > 89.0)
    {
        mPitch = 89.0;
    }

    if (mPitch < -89.0)
    {
        mPitch = -89.0;
    }

    double tYawRadian = glm::radians(mYaw);
    double tPitchRadian = glm::radians(mPitch);
    double tCosPitch = cos(tPitchRadian);

    mFrontVector = glm::normalize(glm::vec3(cos(tYawRadian) * tCosPitch,
                                            sin(tPitchRadian),
                                            sin(tYawRadian) * tCosPitch));

    mRightVector = glm::normalize(glm::cross(mFrontVector, mWorldUpVector));
    mUpVector = glm::normalize(glm::cross(mRightVector, mFrontVector));

    mViewMatrix = glm::lookAt(mPosition, mPosition + mFrontVector, mUpVector);

    calculateViewProjection();
}

void FPSCamera::processMouseMovement(double pXOffset, double pYOffset, bool pConstrainPitch)
{
    double tSensitivity = 0.1;

    pXOffset *= tSensitivity;
    pYOffset *= tSensitivity;

    mYaw += pXOffset;
    mPitch += pYOffset;

    if (pConstrainPitch)
    {
        if (mPitch > 89.0)
        {
            mPitch = 89.0;
        }
        if (mPitch < -89.0)
        {
            mPitch = -89.0;
        }
    }

    update();
}
