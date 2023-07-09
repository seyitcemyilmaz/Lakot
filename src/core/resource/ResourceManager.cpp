#include "ResourceManager.h"

#include "loaders/ModelLoader.h"

ResourceManager* ResourceManager::mInstance = nullptr;

ResourceManager* ResourceManager::getInstance() {
	if (!mInstance) {
		mInstance = new ResourceManager();
	}

	return mInstance;
}

ResourceManager::ResourceManager() { }

void ResourceManager::addModelResource(std::string pModelFilePath) {
	if (mModelResources.contains(pModelFilePath)) {
		std::cout << "Model is already loaded." << std::endl;
		return;
	}

	ModelLoader* tModelLoader = new ModelLoader(pModelFilePath);

	ModelResource* tModelResource = tModelLoader->loadModel();

	if (!tModelResource) {
		std::cout << "Model is not found." << std::endl;
		return;
	}

	mModelResources[pModelFilePath] = tModelResource;

	std::cout << "Model is loaded successfully." << std::endl;
}

ModelResource* ResourceManager::getModelResource(std::string pModelFilePath) {
	if (mModelResources.contains(pModelFilePath)) {
		return mModelResources[pModelFilePath];
	}

	std::cout << "Model Resource: " << pModelFilePath << " is not found." << std::endl;

	return nullptr;
}
