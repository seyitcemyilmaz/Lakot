#include "ResourceManager.h"

#include <spdlog/spdlog.h>

#include "loaders/ModelLoader.h"

ResourceManager* ResourceManager::mInstance = nullptr;

ResourceManager* ResourceManager::getInstance() {
	if (!mInstance) {
		mInstance = new ResourceManager();
	}

	return mInstance;
}

ResourceManager::ResourceManager() = default;

void ResourceManager::addModelResource(const std::string& pModelFilePath) {
	if (mModelResources.contains(pModelFilePath)) {
		spdlog::info("Model is already loaded.");
		return;
	}

	ModelLoader* tModelLoader = new ModelLoader(pModelFilePath);

	ModelResource* tModelResource = tModelLoader->loadModel();

	if (!tModelResource) {
		spdlog::info("Model is not found.");
		return;
	}

	mModelResources[pModelFilePath] = tModelResource;

	spdlog::info("Model is loaded successfully.");
}

ModelResource* ResourceManager::getModelResource(const std::string& pModelFilePath) {
	if (mModelResources.contains(pModelFilePath)) {
		return mModelResources[pModelFilePath];
	}

	spdlog::warn("Model Resource: {0} is not found.", pModelFilePath);

	return nullptr;
}
