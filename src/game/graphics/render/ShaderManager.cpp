#include "ShaderManager.h"

#include <SDL3/SDL_log.h>

using namespace lakot;

ShaderManager::ShaderManager()
{

}

ShaderManager::~ShaderManager()
{

}

void ShaderManager::initialize()
{

}

void ShaderManager::deinitialize()
{
    for (auto& tPair : mPrograms)
    {
        if (tPair.second)
        {
            tPair.second->deinitialize();
            delete tPair.second;
        }
    }

    mPrograms.clear();
}

ShaderProgram* ShaderManager::createProgram(const std::string& pName, const std::string& pVertexSource, const std::string& pFragmentSource)
{
    {
        auto tIterator = mPrograms.find(pName);

        if (tIterator != mPrograms.end())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader program '%s' already exists. Returning the existing one.", pName.c_str());
            return tIterator->second;
        }
    }

    Shader* tVertexShader = new Shader(pVertexSource, ShaderType::eVertex);
    Shader* tFragmentShader = new Shader(pFragmentSource, ShaderType::eFragment);

    ShaderProgram* tProgram = new ShaderProgram(pName, tVertexShader, tFragmentShader);

    try
    {
        tProgram->initialize();

        tVertexShader->deinitialize();
        tFragmentShader->deinitialize();

        delete tVertexShader;
        delete tFragmentShader;

        mPrograms[pName] = tProgram;

        return tProgram;
    }
    catch (const std::exception& tException)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create shader program '%s': %s", pName.c_str(), tException.what());

        delete tProgram;
        delete tVertexShader;
        delete tFragmentShader;

        return nullptr;
    }
}

ShaderProgram* ShaderManager::getProgram(const std::string& pName) const
{
    auto tIterator = mPrograms.find(pName);

    if (tIterator != mPrograms.end())
    {
        return tIterator->second;
    }

    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader program '%s' not found!", pName.c_str());
    return nullptr;
}
