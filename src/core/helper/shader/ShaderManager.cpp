#include "ShaderManager.h"

ShaderManager* ShaderManager::mInstance = nullptr;

ShaderManager* ShaderManager::getInstance()
{
    if (!mInstance)
    {
        mInstance = new ShaderManager();
    }

    return mInstance;
}

ShaderManager::ShaderManager()
    : mActiveShaderProgram(nullptr)
{

}

ShaderProgram* ShaderManager::getShaderProgram(const std::string& pShaderProgramName)
{
    for (size_t i = 0; i < mShaderPrograms.size(); i++)
    {
        if (mShaderPrograms[i]->getName() == pShaderProgramName)
        {
            return mShaderPrograms[i];
        }
    }

    throw "Invalid shader";
}

void ShaderManager::addShaderProgram(ShaderProgram* pShaderProgram)
{
    mShaderPrograms.push_back(pShaderProgram);
    pShaderProgram->init();
}

void ShaderManager::bindShaderProgram(ShaderProgram* pShaderProgram)
{
    if (mActiveShaderProgram != pShaderProgram)
    {
        pShaderProgram->bind();
        mActiveShaderProgram = pShaderProgram;
    }
}

void ShaderManager::deleteShaderPrograms()
{
    for (auto tIterator = mShaderPrograms.begin(); tIterator != mShaderPrograms.end(); tIterator++)
    {
        ShaderProgram* tShaderProgram = *tIterator;
        delete tShaderProgram;
    }

    mShaderPrograms.clear();

    setActiveShaderNull();
}

void ShaderManager::setActiveShaderNull()
{
    mActiveShaderProgram = nullptr;
}
