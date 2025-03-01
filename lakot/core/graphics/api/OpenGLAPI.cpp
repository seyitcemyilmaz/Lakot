#include "OpenGLAPI.h"

#if defined(LAKOT_RENDERER_OPENGL)
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#else
#error "Undefined OpenGL Type."
#endif
#endif

#include <spdlog/spdlog.h>

#include "../../render/RendererFactory.h"

using namespace lakot;

OpenGLAPI::~OpenGLAPI()
{

}

OpenGLAPI::OpenGLAPI()
    : GraphicsAPI()
{
    mType = GraphicsAPIType::eOpenGL;

#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    mOpenGLType = OpenGLType::eCore;
    mVersionMajor = 4;
    mVersionMinor = 6;
#elif defined(LAKOT_RENDERER_OPENGL_TYPE_ES)
    mOpenGLType = OpenGLType::eES;
    mVersionMajor = 3;
    mVersionMinor = 2;
#else
#error "Undefined OpenGL Version Type"
#endif
}

void OpenGLAPI::initialize()
{
    spdlog::info("OpenGL API is initializing.");

    if (mInstance)
    {
        spdlog::error("Graphics API cannot be initialize again.");
        return;
    }

#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
    if (!glfwInit())
    {
        throw "GLFW is not initialized.";
    }
#endif

    mRenderer = RendererFactory::createRenderer();

    if (!mRenderer)
    {
        throw "Renderer could not be created.";
    }

    mInstance = this;

    mIsInitialized = true;

    spdlog::info("OpenGL API is initialized.");
}

void OpenGLAPI::deinitialize()
{
    spdlog::info("OpenGL API is deinitializing.");

    if (mIsInitialized)
    {
        if (mRenderer)
        {
            mRenderer->deinitialize();
        }
#if defined(LAKOT_RENDERER_OPENGL_TYPE_CORE)
        glfwTerminate();
#endif
        mInstance = nullptr;
        mIsInitialized = false;
    }

    spdlog::info("OpenGL API is deinitialized.");
}

OpenGLType OpenGLAPI::getOpenGLType() const
{
    return mOpenGLType;
}
