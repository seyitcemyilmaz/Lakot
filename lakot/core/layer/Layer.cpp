#include "Layer.h"

#include <spdlog/spdlog.h>

#include "../graphics/window/Window.h"
// #include "../graphics/api/GraphicsAPI.h"

using namespace lakot;

Layer* Layer::mInstance = nullptr;

Layer::~Layer()
{

}

Layer::Layer()
    : ALayer()
{

}

Layer* Layer::getInstance()
{
    return mInstance;
}

void Layer::initialize()
{
    spdlog::info("Layer is initializing.");

    mIsInitialized = true;

    mInstance = this;

    spdlog::info("Layer is initialized.");
}

void Layer::deinitialize()
{
    spdlog::info("Layer is deinitializing.");

    // TODO: will be checked.
    mIsInitialized = false;

    mInstance = nullptr;

    spdlog::info("Layer is deinitialized.");
}

void Layer::update()
{
    double tFPS = Window::getInstance()->getFPS();

    spdlog::info("FPS: {0}", tFPS);

    // IRenderer* tRenderer = GraphicsAPI::getInstance()->getRenderer();
}
