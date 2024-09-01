#include "Engine.h"

#include <spdlog/spdlog.h>

#include "GarbageCollectorFactory.h"
#include "../graphics/api/GraphicsAPIFactory.h"
#include "../graphics/window/WindowFactory.h"
#include "../layer/LayerFactory.h"

#include "Logger.h"

using namespace lakot;

Engine* Engine::mEngine = nullptr;

Engine::~Engine() {}

Engine::Engine()
    : AEngine()
{

}

Engine* Engine::getInstance()
{
    return mEngine;
}

void Engine::initialize()
{
    Logger::getInstance()->initialize();

    spdlog::info("Engine is initializing.");

    if (mEngine)
    {
        spdlog::error("Engine cannot be initialize again.");
        return;
    }

    initializeGarbageCollector();

    initializeGraphicsAPI();

    initializeWindow();

    initializeLayer();

    mEngine = this;

    test();

    spdlog::info("Engine is initialized.");
}

void Engine::deinitialize()
{
    spdlog::info("Engine is deinitializing.");

    Layer::getInstance()->deinitialize();
    Window::getInstance()->deinitialize();
    GraphicsAPI::getInstance()->deinitialize();

    spdlog::info("Engine is deinitialized.");
}

void Engine::run()
{
    while (Window::getInstance()->getIsActive())
    {
        GarbageCollector::getInstance()->process();
        Window::getInstance()->update();
        Layer::getInstance()->update();
        Window::getInstance()->nextFrame();
    }
}

void Engine::initializeGarbageCollector()
{
    GarbageCollector* tGarbageCollector = GarbageCollectorFactory::createGarbageCollector();

    if (!tGarbageCollector)
    {
        throw "Garbage collector is not found.";
    }

    tGarbageCollector->initialize();
}

void Engine::initializeGraphicsAPI()
{
    GraphicsAPI* tGraphicsAPI = GraphicsAPIFactory::createGraphicsAPI();

    if (!tGraphicsAPI)
    {
        throw "Graphics API is not found.";
    }

    tGraphicsAPI->initialize();
}

void Engine::initializeWindow()
{
    Window* tWindow = WindowFactory::createWindow();

    if (!tWindow)
    {
        throw "Window is not found.";
    }

    tWindow->initialize();

    bool tIsWindowInitialized = tWindow->getIsInitialized();

    if (!tIsWindowInitialized)
    {
        throw "Window is not initialized.";
    }
}

void Engine::initializeLayer()
{
    Layer* tLayer = LayerFactory::createLayer();

    if (!tLayer)
    {
        throw "Layer is not found.";
    }

    tLayer->initialize();

    bool tIsLayerInitialized = tLayer->getIsInitialized();

    if (!tIsLayerInitialized)
    {
        throw "Layer is not initialized.";
    }
}

void Engine::test()
{

}
