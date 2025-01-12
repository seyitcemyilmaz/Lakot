#include "Engine.h"

#include <spdlog/spdlog.h>

#include <lakot/utilities/FileManager.h>

#include "GarbageCollectorFactory.h"

#include "../graphics/api/GraphicsAPIFactory.h"
#include "../graphics/window/WindowFactory.h"

#include "../layer/LayerFactory.h"

// #include "../resource/loader/ModelLoader.h"

#include "Logger.h"

#if defined(LAKOT_PLATFORM_WINDOWS)     // TODO: will be compatible with linux

#ifdef LAKOT_USE_GPU
extern "C"
{
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdfG
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

// http://developer.amd.com/community/blog/2015/10/02/amd-enduro-system-for-developers/
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#endif

using namespace lakot;

Engine* Engine::mEngine = nullptr;

Engine::~Engine()
{

}

Engine::Engine()
    : Object()
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

    initializeFileManager();

    initializeGarbageCollector();

    initializeGraphicsAPI();

    initializeWindow();

    initializeRenderer();

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
        GarbageCollector::getInstance()->synchronousProcess();
        Window::getInstance()->update();
        if (Window::getInstance()->getIsInitialized())
        {
            Layer::getInstance()->update();
        }
        Window::getInstance()->nextFrame();
    }
}

void Engine::initializeFileManager()
{
    std::string tAssetPath = "";

#if LAKOT_PLATFORM_WINDOWS
    std::filesystem::path tCurrentPath = __FILE__;
    std::filesystem::path tEngineFolderPath = tCurrentPath.parent_path();
    std::filesystem::path tCoreFolderPath = tEngineFolderPath.parent_path();
    std::filesystem::path tIncludePath = tCoreFolderPath.parent_path();
    std::filesystem::path tLakotPath = tIncludePath.parent_path();

    tAssetPath = FileManager::createPath(tLakotPath, "asset");
#endif

    FileManager::setAssetPath(tAssetPath);
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
}

void Engine::initializeLayer()
{
#if !defined(LAKOT_PLATFORM_ANDROID)
    Layer* tLayer = LayerFactory::createLayer();

    if (!tLayer)
    {
        throw "Layer is not found.";
    }

    tLayer->initialize();
#endif
}

void Engine::initializeRenderer()
{
#if !defined(LAKOT_PLATFORM_ANDROID)
    if (GraphicsAPI::getInstance())
    {
        ARenderer* tRenderer = GraphicsAPI::getInstance()->getRenderer();

        if (tRenderer)
        {
            tRenderer->initialize();
        }
    }
#endif
}

void Engine::test()
{
    // ModelLoader* tModelLoader = new ModelLoader();

    // tModelLoader->setPath("C:/Development/Lakot/asset/bmw/scene.gltf");
    // // tModelLoader->setPath("C:/Development/Lakot/asset/servator/source/xenocat_sketchfab.fbx");

    // tModelLoader->load();

    // GarbageCollector::getInstance()->add(tModelLoader, true);
}
