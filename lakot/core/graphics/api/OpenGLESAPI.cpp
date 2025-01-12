#include "OpenGLESAPI.h"

#include <spdlog/spdlog.h>

#include "../../render/RendererFactory.h"

using namespace lakot;

OpenGLESAPI::~OpenGLESAPI()
{

}

OpenGLESAPI::OpenGLESAPI()
    : GraphicsAPI()
{
    mType = GraphicsAPIType::eOpenGLES;
    mVersionMajor = 3;
    mVersionMinor = 2;
}

void OpenGLESAPI::initialize()
{
    spdlog::info("OpenGLES API is initializing.");

    mRenderer = RendererFactory::createRenderer();

    if (!mRenderer)
    {
        throw "Renderer could not be created.";
    }

    mInstance = this;

    mIsInitialized = true;

    spdlog::info("OpenGLES API is initialized.");
}

void OpenGLESAPI::deinitialize()
{

}
