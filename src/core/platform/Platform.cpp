#include "Platform.h"

#include "helper/FileManager.h"

Platform::~Platform()
{
	delete mGraphicsAPI;
}

Platform::Platform(GraphicsAPI* pGraphicsAPI)
	: mGraphicsAPI(pGraphicsAPI)
	, mRenderFunction(nullptr)
	, mProcessInputsFunction(nullptr)
{
	mCurrentTime = 0.0f;
	mPreviousTime = 0.0f;
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

void Platform::setRenderFunction(std::function<void ()> pRenderFunction)
{
	mRenderFunction = pRenderFunction;
}

void Platform::setProcessInputFunction(std::function<void (double)> pProcessInputFunction)
{
	mProcessInputsFunction = pProcessInputFunction;
}
