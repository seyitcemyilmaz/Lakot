#include "Shader.h"

#include <spdlog/spdlog.h>

#include "platform/Platform.h"
#include "helper/FileManager.h"

Shader::~Shader()
{
    glDeleteShader(mId);
}

Shader::Shader(const std::string& pFileLocation, ShaderType pType)
    : mId(UINT_MAX)
    , mType(pType)
    , mFileLocation(pFileLocation)
    , mIsCompiled(false)
{

}

void Shader::compile()
{
    if (mIsCompiled)
    {
        spdlog::error("{0}: Shader is already compiled.", mFileLocation);
        return;
    }

    unsigned int tShaderType = 0;

    if (mType == ShaderType::eVertex)
    {
        tShaderType = GL_VERTEX_SHADER;
    }
    else if (mType == ShaderType::eFragment)
    {
        tShaderType = GL_FRAGMENT_SHADER;
    }
    else if (mType == ShaderType::eGeometry)
    {
        tShaderType = GL_GEOMETRY_SHADER;
    }
    else
    {
        throw "Undefined shader type.";
    }

    mId = glCreateShader(tShaderType);

    std::string tShaderHeader = std::string(LAKOT_GLSL_VERSION) + "\n";

    mContent = tShaderHeader + FileManager::getInstance()->getFileContent(mFileLocation);
    const char* tContentCStr = mContent.c_str();

    glShaderSource(mId, 1, &tContentCStr, nullptr);
    glCompileShader(mId);

    int tIsShaderCompiled = 0;
    glGetShaderiv(mId, GL_COMPILE_STATUS, &tIsShaderCompiled);

    mIsCompiled = tIsShaderCompiled > 0;

    if (!mIsCompiled)
    {
        char tErrorInfo[512];
        glGetShaderInfoLog(mId, 512, nullptr, tErrorInfo);

        spdlog::info(tErrorInfo);
        throw mFileLocation + " does not compiled.";
    }
}

unsigned int Shader::getId() const
{
    return mId;
}

const std::string& Shader::getFileLocation() const
{
    return mFileLocation;
}

const std::string& Shader::getContent() const
{
    return mContent;
}

bool Shader::getIsCompiled() const
{
    return mIsCompiled;
}

unsigned int Shader::compileShader(const std::string& pShaderFilePath, unsigned int tShaderType)
{
    unsigned int tShaderId = glCreateShader(tShaderType);

    std::string tShaderHeader = std::string(LAKOT_GLSL_VERSION) + "\n";

    std::string tShaderFileContent = tShaderHeader + FileManager::getInstance()->getFileContent(pShaderFilePath);
    const char* tShaderFileContentCStr = tShaderFileContent.c_str();

    glShaderSource(tShaderId, 1, &tShaderFileContentCStr, nullptr);
    glCompileShader(tShaderId);

    int tIsShaderCompiled = 0;

    glGetShaderiv(tShaderId, GL_COMPILE_STATUS, &tIsShaderCompiled);

    if (!tIsShaderCompiled)
    {
        char tErrorInfo[512];
        glGetShaderInfoLog(tShaderId, 512, nullptr, tErrorInfo);

        spdlog::info(tErrorInfo);
        throw pShaderFilePath + " does not compiled.";
    }

    return tShaderId;
}
