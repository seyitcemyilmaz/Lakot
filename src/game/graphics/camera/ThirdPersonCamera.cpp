#include "ThirdPersonCamera.h"

#include <algorithm>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

using namespace lakot;

namespace
{
    constexpr float kMinDistance = 3.0f;
    constexpr float kMaxDistance = 15.0f;
}

ThirdPersonCamera::~ThirdPersonCamera()
{

}

ThirdPersonCamera::ThirdPersonCamera()
    : Camera(CameraType::eThirdPerson)
    , mTargetPosition(0.0f, 2.0f, 10.0f)
    , mYaw(270.0)
    , mPitch(25.0)
    , mDistance(8.0f)
    , mMinimumEyeHeight(std::numeric_limits<float>::lowest())
{
    update();
}

void ThirdPersonCamera::update()
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

    // "Front" here means the direction from the camera's eye toward the
    // target, same as FPSCamera's look direction - x/z match FPSCamera's
    // formula exactly (so yaw stays meaningful the same way for
    // sendLocalPlayerState()'s atan2 and for flattened WASD movement), but
    // the vertical sign is flipped: positive pitch here means the camera
    // sits *above* the target looking down (dragging the mouse up raises
    // the camera), matching the classic MMO orbit-camera feel.
    mFrontVector = glm::normalize(glm::vec3(cos(tYawRadian) * tCosPitch,
                                            -sin(tPitchRadian),
                                            sin(tYawRadian) * tCosPitch));

    mRightVector = glm::normalize(glm::cross(mFrontVector, mWorldUpVector));
    mUpVector = glm::normalize(glm::cross(mRightVector, mFrontVector));

    mPosition = mTargetPosition - mFrontVector * mDistance;
    mPosition.y = std::max(mPosition.y, mMinimumEyeHeight);

    mViewMatrix = glm::lookAt(mPosition, mTargetPosition, mUpVector);

    calculateViewProjection();
}

void ThirdPersonCamera::orbit(double pXOffset, double pYOffset)
{
    double tSensitivity = 0.2;

    mYaw += pXOffset * tSensitivity;
    mPitch += pYOffset * tSensitivity;

    update();
}

void ThirdPersonCamera::zoom(float pAmount)
{
    mDistance -= pAmount;

    if (mDistance < kMinDistance)
    {
        mDistance = kMinDistance;
    }

    if (mDistance > kMaxDistance)
    {
        mDistance = kMaxDistance;
    }

    update();
}

void ThirdPersonCamera::setTargetPosition(const glm::vec3& pPosition)
{
    mTargetPosition = pPosition;
    update();
}

const glm::vec3& ThirdPersonCamera::getTargetPosition() const
{
    return mTargetPosition;
}

void ThirdPersonCamera::moveTarget(const glm::vec3& pAmount)
{
    mTargetPosition += pAmount;
    update();
}

void ThirdPersonCamera::setMinimumEyeHeight(float pHeight)
{
    mMinimumEyeHeight = pHeight;
}

void ThirdPersonCamera::setYaw(double pYaw)
{
    mYaw = pYaw;
    update();
}
