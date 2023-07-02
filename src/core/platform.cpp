#include "platform.h"

Platform::Platform(GraphicsAPI* pGraphicsAPI) {
	mGraphicsAPI = pGraphicsAPI;
}

GraphicsAPI* Platform::getGraphicsAPI() {
	return mGraphicsAPI;
}

std::string Platform::getAssetsPath() {
	std::string tLakotAssetsPath = getRootPath() + "/assets";
	return tLakotAssetsPath;
}

std::string Platform::getModelsPath() {
	std::string tLakotModelsPath = getAssetsPath() + "/models";
	return tLakotModelsPath;
}

Platform::~Platform() {
	delete mGraphicsAPI;
}
