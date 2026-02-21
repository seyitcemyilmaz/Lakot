#include "Camera.h"

#include <glm/ext/matrix_clip_space.hpp>

using namespace lakot;

Camera::~Camera()
{

}

Camera::Camera(CameraType pType)
    : mType(pType)
    , mPosition(0.0f, 2.0f, 10.0f)
    , mFrontVector(0.0f, 0.0f, -1.0f)
    , mUpVector(0.0f, 1.0f, 0.0f)
    , mRightVector(1.0f, 0.0f, 0.0f)
    , mWorldUpVector(0.0f, 1.0f, 0.0f)
    , mZoom(45.0f)
    , mNear(0.1f)
    , mFar(100.0f)
{

}

void Camera::updateProjection(int pWidth, int pHeight)
{
    if (pHeight <= 0)
    {
        pHeight = 1;
    }

    float tAspectRatio = static_cast<float>(pWidth) / static_cast<float>(pHeight);

    mProjectionMatrix = glm::perspective(glm::radians(mZoom), tAspectRatio, mNear, mFar);
}

void Camera::calculateViewProjection()
{
    mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}

CameraType Camera::getType() const
{
    return mType;
}

const glm::vec3& Camera::getPosition() const
{
    return mPosition;
}

void Camera::setPosition(const glm::vec3& pPosition)
{
    mPosition = pPosition;
}

const glm::vec3& Camera::getFrontVector() const
{
    return mFrontVector;
}

void Camera::setFrontVector(const glm::vec3& pFrontVector)
{
    mFrontVector = pFrontVector;
}

const glm::vec3& Camera::getUpVector() const
{
    return mUpVector;
}

void Camera::setUpVector(const glm::vec3& pUpVector)
{
    mUpVector = pUpVector;
}

const glm::vec3& Camera::getRightVector() const
{
    return mRightVector;
}

void Camera::setRightVector(const glm::vec3& pRightVector)
{
    mRightVector = pRightVector;
}

const glm::vec3& Camera::getWorldUpVector() const
{
    return mWorldUpVector;
}

void Camera::setWorldUpVector(const glm::vec3& pWorldUpVector)
{
    mWorldUpVector = pWorldUpVector;
}

float Camera::getZoom() const
{
    return mZoom;
}

void Camera::setZoom(float pZoom)
{
    mZoom = pZoom;
}

float Camera::getNear() const
{
    return mNear;
}

void Camera::setNear(float pNear)
{
    mNear = pNear;
}

float Camera::getFar() const
{
    return mFar;
}

void Camera::setFar(float pFar)
{
    mFar = pFar;
}

void Camera::changePosition(const glm::vec3& pAmount)
{
    mPosition += pAmount;
}

const glm::mat4& Camera::getViewMatrix() const
{
    return mViewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() const
{
    return mProjectionMatrix;
}

const glm::mat4& Camera::getViewProjectionMatrix() const
{
    return mViewProjectionMatrix;
}
