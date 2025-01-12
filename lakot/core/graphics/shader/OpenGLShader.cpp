#include "OpenGLShader.h"

#ifdef LAKOT_RENDERER_OPENGL
#include <GL/glew.h>
#endif

#ifdef LAKOT_RENDERER_OPENGLES
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
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

#if defined(LAKOT_RENDERER_OPENGL)
   std::string tVersionString = std::to_string(tVersionMajor * 100 + tVersionMinor * 10);
   std::string tShaderHeader = "#version " + tVersionString + " core\n";
#elif defined(LAKOT_RENDERER_OPENGLES)
    std::string tShaderHeader = "#version 320 es\n";
#elif
#error "Undefined shader header."
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
