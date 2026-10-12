#include "Shader.h"

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <stdexcept>

using namespace lakot;

Shader::~Shader()
{

}

Shader::Shader(const std::string& pFileContent, ShaderType pType)
    : mIsInitialized(false)
    , mId(0)
    , mType(pType)
    , mFileContent(pFileContent)
{
}

void Shader::initialize()
{
    if (mIsInitialized)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader is already compiled.");
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
    else
    {
        throw std::runtime_error("Undefined shader type.");
    }

    mId = glCreateShader(tShaderType);

#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    std::string tShaderHeader = "#version 460 core\n";
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
    std::string tShaderHeader = "#version 320 es\n";
    tShaderHeader += "precision highp float;\n";
#else
#error "Undefined shader header."
#endif

    std::string tSource = tShaderHeader + mFileContent;
    const char* tContentCStr = tSource.c_str();

    glShaderSource(mId, 1, &tContentCStr, nullptr);
    glCompileShader(mId);

    int tIsShaderCompiled = 0;
    glGetShaderiv(mId, GL_COMPILE_STATUS, &tIsShaderCompiled);

    mIsInitialized = tIsShaderCompiled > 0;

    if (!mIsInitialized)
    {
        char tErrorInfo[512];
        glGetShaderInfoLog(mId, 512, nullptr, tErrorInfo);

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader compilation failed. Error: %s", tErrorInfo);
        glDeleteShader(mId);
        mId = 0;
        throw std::runtime_error("A shader did not compile.");
    }
}

void Shader::deinitialize()
{
    if (mIsInitialized)
    {
        glDeleteShader(mId);
        mIsInitialized = false;
    }
}

unsigned int Shader::getId() const
{
    return mId;
}
