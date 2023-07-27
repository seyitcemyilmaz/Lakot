#include "Platform.h"

#include "helper/FileManager.h"

Platform::Platform(GraphicsAPI* pGraphicsAPI) : mGraphicsAPI(pGraphicsAPI) {
	mCurrentTime = 0.0f;
	mPreviousTime = 0.0f;
}

GraphicsAPI* Platform::getGraphicsAPI() {
	return mGraphicsAPI;
}

std::string Platform::getAssetsPath() {
	std::string tLakotAssetsPath = FileManager::getInstance()->createPath(getRootPath(), "assets");
	return tLakotAssetsPath;
}

std::string Platform::getModelsPath() {
	std::string tLakotModelsPath = FileManager::getInstance()->createPath(getAssetsPath(), "models");
	return tLakotModelsPath;
}

Platform::~Platform() {
	delete mGraphicsAPI;
}
