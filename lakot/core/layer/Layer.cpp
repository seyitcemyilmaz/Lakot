#include "Layer.h"

#include <spdlog/spdlog.h>

#include "../graphics/window/Window.h"
#include "../graphics/api/GraphicsAPI.h"

#include "../scene/Scene.h"

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

    mInstance = this;

    mScene = new Scene();
    mScene->initialize();

    spdlog::info("Layer is initialized.");
}

void Layer::deinitialize()
{
    spdlog::info("Layer is deinitializing.");

    mInstance = nullptr;
    delete mScene;

    spdlog::info("Layer is deinitialized.");
}

void Layer::update()
{
    mScene->update();

    // ARenderer* tRenderer = GraphicsAPI::getInstance()->getRenderer();


}
