#include "OpenGLShaderProgram.h"

#ifdef LAKOT_RENDERER_OPENGL
#include <GL/glew.h>
#endif

#ifdef LAKOT_RENDERER_OPENGLES
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#endif

#include <glm/gtc/type_ptr.hpp>

#include <spdlog/spdlog.h>

using namespace lakot;

OpenGLShaderProgram::~OpenGLShaderProgram()
{

}

OpenGLShaderProgram::OpenGLShaderProgram(const std::string& pName, OpenGLShader* pVertexShader, OpenGLShader* pFragmentShader)
    : AShaderProgram(pName, pVertexShader, pFragmentShader)
{

}

void OpenGLShaderProgram::initialize()
{
    unsigned int tVertexShaderId = UINT_MAX;
    unsigned int tFragmentShaderId = UINT_MAX;

    if (mVertexShader)
    {
        mVertexShader->initialize();

        tVertexShaderId = mVertexShader->getId();
    }

    if (mFragmentShader)
    {
        mFragmentShader->initialize();

        tFragmentShaderId = mFragmentShader->getId();
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

void OpenGLShaderProgram::deinitialize()
{
    glDeleteProgram(mId);
}

void OpenGLShaderProgram::bind()
{
    glUseProgram(mId);
}

void OpenGLShaderProgram::setBool(const std::string& pUniformName, bool pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glUniform1i(tUniform->location, pValue);
}

void OpenGLShaderProgram::setInt(const std::string& pUniformName, int pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glUniform1i(tUniform->location, pValue);
}

void OpenGLShaderProgram::setVec3(const std::string& pUniformName, const glm::vec3& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glUniform3fv(tUniform->location, 1, glm::value_ptr(pValue));
}

void OpenGLShaderProgram::setMat4(const std::string& pUniformName, const glm::mat4& pValue)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glUniformMatrix4fv(tUniform->location, 1, false, glm::value_ptr(pValue));
}

void OpenGLShaderProgram::setMat4Array(const std::string& pUniformName, const glm::mat4* pArray, unsigned int pCount)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glUniformMatrix4fv(tUniform->location, pCount, GL_FALSE, glm::value_ptr(*pArray));
}

void OpenGLShaderProgram::setTexture(const std::string& pUniformName, unsigned int pUnit, int pTextureId)
{
    ShaderUniform* tUniform = getUniform(pUniformName);

    if (!tUniform)
    {
        spdlog::error("Uniform ${0}$ is not found in ${1}$ shader program.", pUniformName, mName);
        return;
    }

    glActiveTexture(GL_TEXTURE0 + pUnit);
    glBindTexture(GL_TEXTURE_2D, pTextureId);
    glUniform1i(tUniform->location, pUnit);
}

ShaderUniform* OpenGLShaderProgram::createUniform(const std::string& pName)
{
    ShaderUniform* tUniform = nullptr;

    int tLocation = glGetUniformLocation(mId, pName.c_str());

    if (!(tLocation < 0))
    {
        tUniform = new ShaderUniform(pName);
        tUniform->location = tLocation;
    }

    mUniforms[pName] = tUniform;

    return tUniform;
}
