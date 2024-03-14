#include "ShaderProgram.h"

#include "platform/Platform.h"

#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

ShaderProgram::~ShaderProgram()
{
    if (mVertexShader)
    {
        delete mVertexShader;
        mVertexShader = nullptr;
    }

    if (mFragmentShader)
    {
        delete mFragmentShader;
        mFragmentShader = nullptr;
    }

    if (mGeometryShader)
    {
        delete mGeometryShader;
        mGeometryShader = nullptr;
    }

    for (auto tIterator = mUniforms.begin(); tIterator != mUniforms.end(); tIterator++)
    {
        const std::string& tUniformName = tIterator->first;

        ShaderUniform* tUniform = tIterator->second;
        delete tUniform;

        mUniforms[tUniformName] = nullptr;
    }

    mUniforms.clear();

    glDeleteProgram(mId);
}

ShaderProgram::ShaderProgram(const std::string& pName, Shader* pVertexShader, Shader* pFragmentShader, Shader* pGeometryShader)
    : mName(pName)
    , mVertexShader(pVertexShader)
    , mFragmentShader(pFragmentShader)
    , mGeometryShader(pGeometryShader)
{

}

void ShaderProgram::init()
{
    unsigned int tVertexShaderId = UINT_MAX;
    unsigned int tFragmentShaderId = UINT_MAX;
    unsigned int tGeometryShaderId = UINT_MAX;

    if (mVertexShader)
    {
        mVertexShader->compile();

        if (mVertexShader->getIsCompiled())
        {
            tVertexShaderId = mVertexShader->getId();
        }
    }

    if (mFragmentShader)
    {
        mFragmentShader->compile();

        if (mFragmentShader->getIsCompiled())
        {
            tFragmentShaderId = mFragmentShader->getId();
        }
    }

    if (mGeometryShader)
    {
        mGeometryShader->compile();

        if (mGeometryShader->getIsCompiled())
        {
            tGeometryShaderId = mGeometryShader->getId();
        }
    }

    mId = glCreateProgram();

    if (tVertexShaderId != UINT_MAX)
    {
        glAttachShader(mId, tVertexShaderId);
    }

    if (tFragmentShaderId != UINT_MAX)
    {
        glAttachShader(mId, tFragmentShaderId);
    }

    if (tGeometryShaderId != UINT_MAX)
    {
        glAttachShader(mId, tGeometryShaderId);
    }

    glLinkProgram(mId);

    int tIsShaderProgramLinked = 0;

    glGetProgramiv(mId, GL_LINK_STATUS, &tIsShaderProgramLinked);

    if (!tIsShaderProgramLinked)
    {
        char tErrorInfo[512];
        glGetProgramInfoLog(mId, 512, nullptr, tErrorInfo);
        spdlog::info(tErrorInfo);
        throw "Linking error in shader program:" + std::string(tErrorInfo);
    }
}

void ShaderProgram::bind()
{
    glUseProgram(mId);
}

const std::string& ShaderProgram::getName()
{
    return mName;
}

void ShaderProgram::setBool(const std::string& pUniformName, bool pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glUniform1i(tUniform->getLocation(), pValue);
}

void ShaderProgram::setInt(const std::string& pUniformName, int pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glUniform1i(tUniform->getLocation(), pValue);
}

void ShaderProgram::setVec3(const std::string& pUniformName, const glm::vec3& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glUniform3fv(tUniform->getLocation(), 1, glm::value_ptr(pValue));
}

void ShaderProgram::setMat4(const std::string& pUniformName, const glm::mat4& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glUniformMatrix4fv(tUniform->getLocation(), 1, false, glm::value_ptr(pValue));
}

void ShaderProgram::setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glUniformMatrix4fv(tUniform->getLocation(), pCount, GL_FALSE, glm::value_ptr(*pArray));
}

void ShaderProgram::setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    glActiveTexture(GL_TEXTURE0 + pUnit);
    glBindTexture(GL_TEXTURE_2D, pTextureId);
    glUniform1i(tUniform->getLocation(), pUnit);
}

ShaderUniform* ShaderProgram::getUniform(const std::string& pName)
{
    if (mUniforms.contains(pName))
    {
        return mUniforms[pName];
    }

    ShaderUniform* tUniform = createUniform(pName);

    return tUniform;
}

ShaderUniform* ShaderProgram::createUniform(const std::string& pName)
{
    ShaderUniform* tUniform = nullptr;

    tUniform = new ShaderUniform(pName);

    int tLocation = glGetUniformLocation(mId, pName.c_str());
    tUniform->setLocation(tLocation);

    mUniforms[pName] = tUniform;

    return tUniform;
}
