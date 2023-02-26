#include "shadermanager.h"

ShaderManager* ShaderManager::mInstance = nullptr;

ShaderManager::ShaderManager() {
    setActiveShaderNull();
}

ShaderManager* ShaderManager::getInstance() {
    if (!mInstance) {
        mInstance = new ShaderManager();
    }

    return mInstance;
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
