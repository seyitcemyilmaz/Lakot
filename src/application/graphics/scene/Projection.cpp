#include "Projection.h"

#include <glm/gtc/matrix_transform.hpp>

Projection::~Projection()
{

}

Projection::Projection()
    : mX(0)
    , mY(0)
    , mWidth(0)
    , mHeight(0)
    , mFarPlaneDistance(LAKOT_DEFAULT_FAR_PLANE_DISTANCE)
    , mNearPlaneDistance(LAKOT_DEFAULT_NEAR_PLANE_DISTANCE)
    , mZoom(LAKOT_DEFAULT_ZOOM)
{

}

void Projection::onResize(double pX, double pY, double pWidth, double pHeight)
{
    mX = pX;
    mY = pY;

    mWidth = pWidth;
    mHeight = pHeight;

    onProjectionChanged();
}

const glm::mat4& Projection::getProjectionMatrix() const
{
    return mProjectionMatrix;
}

double Projection::getX() const
{
    return mX;
}

double Projection::getY() const
{
    return mY;
}

double Projection::getWidth() const
{
    return mWidth;
}

double Projection::getHeight() const
{
    return mHeight;
}

double Projection::getFarPlaneDistance() const
{
    return mFarPlaneDistance;
}

double Projection::getNearPlaneDistance() const
{
    return mNearPlaneDistance;
}

double Projection::getZoom() const
{
    return mZoom;
}

void Projection::setX(double pX)
{
    mX = pX;

    onProjectionChanged();
}

void Projection::setY(double pY)
{
    mY = pY;

    onProjectionChanged();
}

void Projection::setWidth(double pWidth)
{
    mWidth = pWidth;

    onProjectionChanged();
}

void Projection::setHeight(double pHeight)
{
    mHeight = pHeight;

    onProjectionChanged();
}

void Projection::setFarPlaneDistance(double pFarPlaneDistance)
{
    mFarPlaneDistance = pFarPlaneDistance;

    onProjectionChanged();
}

void Projection::setNearPlaneDistance(double pNearPlaneDistance)
{
    mNearPlaneDistance = pNearPlaneDistance;

    onProjectionChanged();
}

void Projection::setZoom(double pZoom)
{
    mZoom = pZoom;

    if (mZoom < LAKOT_MIN_ZOOM)
    {
        mZoom = LAKOT_MIN_ZOOM;
    }

    if (mZoom > LAKOT_MAX_ZOOM)
    {
        mZoom = LAKOT_MAX_ZOOM;
    }

    onProjectionChanged();
}

void Projection::changeZoom(double pAmount)
{
    mZoom -= pAmount;

    if (mZoom < LAKOT_MIN_ZOOM)
    {
        mZoom = LAKOT_MIN_ZOOM;
    }

    if (mZoom > LAKOT_MAX_ZOOM)
    {
        mZoom = LAKOT_MAX_ZOOM;
    }

    onProjectionChanged();
}

void Projection::onProjectionChanged()
{
    mProjectionMatrix = glm::perspective(glm::radians(mZoom), (mWidth / mHeight), mNearPlaneDistance, mFarPlaneDistance);
}
