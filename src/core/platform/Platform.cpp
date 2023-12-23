#include "Platform.h"

#include "helper/FileManager.h"

Platform::~Platform()
{
    delete mGraphicsAPI;
}

Platform::Platform(GraphicsAPI* pGraphicsAPI)
    : mGraphicsAPI(pGraphicsAPI)
    , mCurrentTime(0.0)
    , mPreviousTime(0.0)
    , mTimeDifference(0.0)
    , mRenderFunction(nullptr)
    , mProcessInputsFunction(nullptr)
{

}

GraphicsAPI* Platform::getGraphicsAPI()
{
    return mGraphicsAPI;
}

std::string Platform::getAssetsPath()
{
    std::string tLakotAssetsPath = FileManager::getInstance()->createPath(getRootPath(), "assets");
    return tLakotAssetsPath;
}

std::string Platform::getModelsPath()
{
    std::string tLakotModelsPath = FileManager::getInstance()->createPath(getAssetsPath(), "models");
    return tLakotModelsPath;
}

double Platform::getTimeDifference()
{
    return mTimeDifference;
}

void Platform::setRenderFunction(std::function<void ()> pRenderFunction)
{
    mRenderFunction = pRenderFunction;
}

void Platform::setProcessInputFunction(std::function<void ()> pProcessInputFunction)
{
    mProcessInputsFunction = pProcessInputFunction;
}
