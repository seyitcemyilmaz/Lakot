#include "OpenGLAPI.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
    mVersionMajor = 4;
    mVersionMinor = 6;
}

void OpenGLAPI::initialize()
{
    spdlog::info("OpenGL API is initializing.");

    if (mInstance)
    {
        spdlog::error("Graphics API cannot be initialize again.");
        return;
    }

    if (!glfwInit())
    {
        throw "GLFW is not initialized.";
    }

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

        glfwTerminate();
        mInstance = nullptr;
        mIsInitialized = false;
    }

    spdlog::info("OpenGL API is deinitialized.");
}
