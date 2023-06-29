#include "shadermanager.h"

ShaderManager* ShaderManager::mInstance = nullptr;

ShaderManager* ShaderManager::getInstance() {
	if (!mInstance) {
		mInstance = new ShaderManager();
	}

	return mInstance;
}

ShaderManager::ShaderManager() {
	setActiveShaderNull();
}

void ShaderManager::addShader(std::string pShaderName, Shader* pShader) {
	mShaders[pShaderName] = pShader;
}

void ShaderManager::bindShader(std::string pShaderName) {
	if (!mShaders.contains(pShaderName)) {
		throw "Invalid shader name.";
	}

	Shader* tShader = mShaders[pShaderName];

	if (!tShader) {
		throw "Shader cannot be nullptr.";
	}

	Shader* tActiveShader = mActiveShader.second;

	if (tActiveShader != tShader) {
		tShader->bind();
		mActiveShader = std::make_pair(pShaderName, tShader);
	}
}

void ShaderManager::setActiveShaderNull() {
	mActiveShader = std::make_pair("", nullptr);
}

void ShaderManager::deleteShaders() {
	for (auto tIterator = mShaders.begin(); tIterator != mShaders.end(); tIterator++) {
		Shader* tShader = tIterator->second;
		delete tShader;
	}

	mShaders.clear();

	setActiveShaderNull();
}

void ShaderManager::setProjectionMatrix(glm::mat4& tProjectionMatrix) {
	Shader* tActiveShader = mActiveShader.second;

	if (!tActiveShader) {
		return;
	}

	int tProjectionMatrixLocation = tActiveShader->getProjectionMatrixLocation();

	if (tProjectionMatrixLocation == -1) {
		std::cout << "Projection matrix location is not found." << std::endl;
		return;
	}

	glUniformMatrix4fv(tProjectionMatrixLocation, 1, false, glm::value_ptr(tProjectionMatrix));
}

void ShaderManager::setViewMatrix(glm::mat4& tViewMatrix) {
	Shader* tActiveShader = mActiveShader.second;

	if (!tActiveShader) {
		return;
	}

	int tViewMatrixLocation = tActiveShader->getViewMatrixLocation();

	if (tViewMatrixLocation == -1) {
		std::cout << "View matrix location is not found." << std::endl;
		return;
	}

	glUniformMatrix4fv(tViewMatrixLocation, 1, false, glm::value_ptr(tViewMatrix));
}

void ShaderManager::setModelMatrix(glm::mat4& tModelMatrix) {
	Shader* tActiveShader = mActiveShader.second;

	if (!tActiveShader) {
		return;
	}

	int tModelMatrixLocation = tActiveShader->getModelMatrixLocation();

	if (tModelMatrixLocation == -1) {
		std::cout << "Model matrix location is not found." << std::endl;
		return;
	}

	glUniformMatrix4fv(tModelMatrixLocation, 1, false, glm::value_ptr(tModelMatrix));
}
