#include "AWindow.h"

using namespace lakot;

AWindow::~AWindow()
{
    mOnResizeFunction = nullptr;
}

AWindow::AWindow()
    : Object()
    , mIsInitialized(false)
    , mIsActive(true)
    , mWidth(0)
    , mHeight(0)
    , mTimeDifference(0.0)
    , mCurrentTime(0.0)
    , mPreviousTime(0.0)
    , mOnResizeFunction(nullptr)
{

}

bool AWindow::getIsInitialized() const
{
    return mIsInitialized;
}

bool AWindow::getIsActive() const
{
    return mIsActive;
}

unsigned int AWindow::getWidth() const
{
    return mWidth;
}

void AWindow::setWidth(unsigned int pWidth)
{
    mWidth = pWidth;
}

unsigned int AWindow::getHeight() const
{
    return mHeight;
}

void AWindow::setHeight(unsigned int pHeight)
{
    mHeight = pHeight;
}

const std::string& AWindow::getTitle() const
{
    return mTitle;
}

void AWindow::setTitle(const std::string& pTitle)
{
    mTitle = pTitle;
}

double AWindow::getTimeDifference() const
{
    return mTimeDifference;
}

double AWindow::getFPS() const
{
    return mFPS;
}

void AWindow::setOnResizeFunction(const std::function<void (double, double)>& pOnResizeFunction)
{
    mOnResizeFunction = pOnResizeFunction;
}
