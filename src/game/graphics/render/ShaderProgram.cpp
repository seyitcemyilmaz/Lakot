#include "ShaderProgram.h"

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <glm/gtc/type_ptr.hpp>

using namespace lakot;

ShaderProgram::~ShaderProgram()
{
    deinitialize();
}

ShaderProgram::ShaderProgram(const std::string& pName, Shader* pVertexShader, Shader* pFragmentShader)
    : mId(0)
    , mName(pName)
    , mVertexShader(pVertexShader)
    , mFragmentShader(pFragmentShader)
{
}

void ShaderProgram::initialize()
{
    if (mVertexShader) mVertexShader->initialize();
    if (mFragmentShader) mFragmentShader->initialize();

    mId = glCreateProgram();

    if (mVertexShader) glAttachShader(mId, mVertexShader->getId());
    if (mFragmentShader) glAttachShader(mId, mFragmentShader->getId());

    glLinkProgram(mId);

    int tIsShaderProgramLinked = 0;
    glGetProgramiv(mId, GL_LINK_STATUS, &tIsShaderProgramLinked);

    if (!tIsShaderProgramLinked)
    {
        char tErrorInfo[512];
        glGetProgramInfoLog(mId, 512, nullptr, tErrorInfo);

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Linking error in shader program (%s): %s", mName.c_str(), tErrorInfo);
        throw std::string("Linking error in shader program.");
    }
}

void ShaderProgram::deinitialize()
{
    if (mId != 0)
    {
        glDeleteProgram(mId);
        mId = 0;
    }

    for (auto& tPair : mUniforms)
    {
        delete tPair.second;
    }

    mUniforms.clear();
}

void ShaderProgram::bind() const
{
    glUseProgram(mId);
}

void ShaderProgram::unbind() const
{
    glUseProgram(0);
}

ShaderUniform* ShaderProgram::createUniform(const std::string& pName)
{
    int tLocation = glGetUniformLocation(mId, pName.c_str());

    if (tLocation < 0)
    {
        return nullptr;
    }

    ShaderUniform* tUniform = new ShaderUniform(pName);
    tUniform->location = tLocation;

    mUniforms[pName] = tUniform;

    return tUniform;
}

ShaderUniform* ShaderProgram::getUniform(const std::string& pName)
{
    auto tIterator = mUniforms.find(pName);

    if (tIterator != mUniforms.end())
    {
        return tIterator->second;
    }

    return createUniform(pName);
}

void ShaderProgram::setBool(const std::string& pUniformName, bool pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glUniform1i(tUniform->location, pValue);
}

void ShaderProgram::setInt(const std::string& pUniformName, int pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glUniform1i(tUniform->location, pValue);
}

void ShaderProgram::setVec3(const std::string& pUniformName, const glm::vec3& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glUniform3fv(tUniform->location, 1, glm::value_ptr(pValue));
}

void ShaderProgram::setMat4(const std::string& pUniformName, const glm::mat4& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glUniformMatrix4fv(tUniform->location, 1, GL_FALSE, glm::value_ptr(pValue));
}

void ShaderProgram::setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glUniformMatrix4fv(tUniform->location, pCount, GL_FALSE, glm::value_ptr(*pArray));
}

void ShaderProgram::setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Uniform %s is not found in %s shader program.", pUniformName.c_str(), mName.c_str());
        return;
    }

    glActiveTexture(GL_TEXTURE0 + pUnit);
    glBindTexture(GL_TEXTURE_2D, pTextureId);
    glUniform1i(tUniform->location, pUnit);
}
