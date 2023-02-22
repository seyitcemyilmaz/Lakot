#include "shadermanager.h"

ShaderManager::ShaderManager() {
    mActiveShader = nullptr;
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

    if (mActiveShader != tShader) {
        mActiveShader = tShader;
        mActiveShader->bind();
    }
}

void ShaderManager::bindShader(Shader* pShader) {
    if (!pShader) {
        throw "Shader cannot be nullptr.";
    }

    if (pShader != mActiveShader) {
        mActiveShader = pShader;
        mActiveShader->bind();
    }
}

ShaderManager::~ShaderManager() {
    for (auto tIterator = mShaders.begin(); tIterator != mShaders.end(); tIterator++) {
        Shader* tShader = tIterator->second;
        delete tShader;
    }
    mShaders.clear();
}
