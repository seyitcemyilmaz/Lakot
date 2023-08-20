#include "ShaderManager.h"

ShaderManager* ShaderManager::mInstance = nullptr;

ShaderManager* ShaderManager::getInstance() {
	if (!mInstance) {
		mInstance = new ShaderManager();
	}

	return mInstance;
}

ShaderManager::ShaderManager() = default;

void ShaderManager::addShader(IShader* pShader) {
	mShaders.push_back(pShader);
}

void ShaderManager::bindShader(IShader* pShader) {
	if (mActiveShader != pShader) {
		pShader->bind();
		mActiveShader = pShader;
	}
}

IShader* ShaderManager::getShader(ShaderName pShaderName) {
	for (size_t i = 0; i < mShaders.size(); i++) {
		if (mShaders[i]->getShaderName() == pShaderName) {
			return mShaders[i];
		}
	}

	throw "Invalid shader";
}

void ShaderManager::setActiveShaderNull() {
	mActiveShader = nullptr;
}

void ShaderManager::deleteShaders() {
	for (auto tShaderElement = mShaders.begin(); tShaderElement != mShaders.end(); tShaderElement++) {
		delete *tShaderElement;
	}

	mShaders.clear();

	setActiveShaderNull();
}
