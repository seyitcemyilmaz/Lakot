#include "Camera.h"

Camera::Camera(const glm::vec3& pPosition)
    : mPosition(pPosition)
    , mFrontVector(LAKOT_CAMERA_FRONT)
    , mWorldUp(LAKOT_CAMERA_WORLD_UP)
    , mYaw(LAKOT_DEFAULT_CAMERA_YAW)
    , mPitch(LAKOT_DEFAULT_CAMERA_PITCH)
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(mPosition, mPosition + mFrontVector, mUpVector);
}

void Camera::updateCameraVectors()
{
    mFrontVector = glm::normalize(glm::vec3(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)),
                                            sin(glm::radians(mPitch)),
                                            sin(glm::radians(mYaw)) * cos(glm::radians(mPitch))));
    mRightVector = glm::normalize(glm::cross(mFrontVector, mWorldUp));
    mUpVector = glm::normalize(glm::cross(mRightVector, mFrontVector));
}

const glm::vec3& Camera::getPosition() const
{
    return mPosition;
}

const glm::vec3& Camera::getFrontVector() const
{
    return mFrontVector;
}

const glm::vec3& Camera::getUpVector() const
{
    return mUpVector;
}

const glm::vec3& Camera::getRightVector() const
{
    return mRightVector;
}

void Camera::setPosition(const glm::vec3& pPosition)
{
    mPosition = pPosition;
}

void Camera::changePosition(const glm::vec3& pAmount)
{
    mPosition += pAmount;
}

void Camera::changeYaw(double pAmount)
{
    mYaw += pAmount;
}

void Camera::changePitch(double pAmount)
{
    mPitch += pAmount;

    if (mPitch > LAKOT_CAMERA_MAX_PITCH)
    {
        mPitch = LAKOT_CAMERA_MAX_PITCH;
    }

    if (mPitch < LAKOT_CAMERA_MIN_PITCH)
    {
        mPitch = LAKOT_CAMERA_MIN_PITCH;
    }
}
