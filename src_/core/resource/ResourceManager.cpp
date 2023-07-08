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

void ResourceManager::addModelResource(std::string pModelFile) {
	if (mModelResources.contains(pModelFile)) {
		std::cout << "Model is already loaded." << std::endl;
		return;
	}

	ModelLoader* tModelLoader = new ModelLoader(pModelFile);

	if (!tModelLoader->loadModel()) {
		std::cout << "Model is not found." << std::endl;
		return;
	}

	std::cout << "Model is loaded successfully." << std::endl;
}
