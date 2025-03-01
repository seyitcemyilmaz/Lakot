#include "OpenGLShader.h"

#if defined(LAKOT_RENDERER_OPENGL)
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
#include <GL/glew.h>
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#else
#error "Undefined OpenGL Type."
#endif
#endif

#include <spdlog/spdlog.h>

#include <lakot/utilities/FileManager.h>

#include "../../graphics/api/GraphicsAPI.h"

using namespace lakot;

OpenGLShader::~OpenGLShader()
{

}

OpenGLShader::OpenGLShader(const std::string& pFileContent, ShaderType pType)
    : AShader()
    , mIsInitialized(false)
    , mFileContent(pFileContent)
{
    mType = pType;
}

void OpenGLShader::initialize()
{
    if (mIsInitialized)
    {
        spdlog::error("Shader is already compiled.");
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
        throw "Undefined shader type.";
    }

    mId = glCreateShader(tShaderType);

    int tVersionMajor = GraphicsAPI::getInstance()->getVersionMajor();
    int tVersionMinor = GraphicsAPI::getInstance()->getVersionMinor();

    std::string tVersionString = std::to_string(tVersionMajor * 100 + tVersionMinor * 10);

#if defined(LAKOT_RENDERER_OPENGL)
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    std::string tShaderHeader = "#version " + tVersionString + " core\n";
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
    std::string tShaderHeader = "#version " + tVersionString + " es\n";
    tShaderHeader += "precision highp float;\n";
#elif
#error "Undefined shader header."
#endif
#endif

    mFileContent = tShaderHeader + mFileContent;

    const char* tContentCStr = mFileContent.c_str();

    glShaderSource(mId, 1, &tContentCStr, nullptr);
    glCompileShader(mId);

    int tIsShaderCompiled = 0;
    glGetShaderiv(mId, GL_COMPILE_STATUS, &tIsShaderCompiled);

    mIsInitialized = tIsShaderCompiled > 0;

    if (!mIsInitialized)
    {
        char tErrorInfo[512];
        glGetShaderInfoLog(mId, 512, nullptr, tErrorInfo);

        spdlog::info("{0}", mFileContent);

        spdlog::info(tErrorInfo);
        throw "A shader does not compiled.";
    }
}

void OpenGLShader::deinitialize()
{

}
