#include "Engine.h"

#include "scene/LoginScene.h"
// #include "scene/WorldScene.h"

using namespace lakot;

Engine& Engine::getInstance()
{
    static Engine tInstance;
    return tInstance;
}

void Engine::initialize()
{
    SDL_Log("Engine is initalizing.");

    mLastTime = SDL_GetPerformanceCounter();

    mNetworkManager.start();

    mShaderManager.initialize();

    mGuiLayer.initialize(mWindow, mGLContext);

    mSceneManager.setNextScene(std::make_unique<LoginScene>(*this));
    // mSceneManager.setNextScene(std::make_unique<WorldScene>(*this));

    SDL_Log("Engine is initialized.");
}

void Engine::deinitialize()
{
    mShaderManager.deinitialize();
    mGuiLayer.deinitialize();
}

void Engine::render()
{
    mNetworkManager.update();

    uint64_t tCurrentTime = SDL_GetPerformanceCounter();
    uint64_t tFrequency = SDL_GetPerformanceFrequency();
    mFrameTime = static_cast<double>(tCurrentTime - mLastTime) / static_cast<double>(tFrequency);

    if (mFrameTime > 0.25)
    {
        mFrameTime = 0.25;
    }

    mLastTime = tCurrentTime;
    mAccumulator += mFrameTime;

    while (mAccumulator >= mFixedDeltaTime)
    {
        mSceneManager.update(mFixedDeltaTime);
        mAccumulator -= mFixedDeltaTime;
    }

    mGarbageCollector.executeSynchronousTasks();

    draw();
}

SDL_Window* Engine::getWindow() const
{
    return mWindow;
}

void Engine::setWindow(SDL_Window* pWindow)
{
    mWindow = pWindow;
}

SDL_GLContext Engine::getGLContext() const
{
    return mGLContext;
}

void Engine::setGLContext(SDL_GLContext pGLContext)
{
    mGLContext = pGLContext;
}

NetworkManager& Engine::getNetworkManager()
{
    return mNetworkManager;
}

SceneManager& Engine::getSceneManager()
{
    return mSceneManager;
}

ShaderManager& Engine::getShaderManager()
{
    return mShaderManager;
}

GuiLayer& Engine::getGuiLayer()
{
    return mGuiLayer;
}

SDL_AppResult Engine::eventHandler(SDL_Event* pEvent)
{
    if (pEvent->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    if (pEvent->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    mGuiLayer.handleEvent(pEvent);

    if (mSceneManager.handleEvent(pEvent))
    {
        return SDL_APP_CONTINUE;
    }

    if (pEvent->type == SDL_EVENT_KEY_DOWN)
    {
        if (pEvent->key.key == SDLK_ESCAPE)
        {
            return SDL_APP_SUCCESS;
        }
    }

    return SDL_APP_CONTINUE;
}

Engine::Engine()
    : mWindow(nullptr)
    , mGLContext(nullptr)
    , mLastTime(0)
    , mAccumulator(0.0)
    , mFixedDeltaTime(1.0 / 60.0)
{

}

Engine::~Engine()
{

}

void Engine::draw()
{
    int tWidth;
    int tHeight;

    SDL_GetWindowSizeInPixels(mWindow, &tWidth, &tHeight);
    glViewport(0, 0, tWidth, tHeight);

    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mGuiLayer.begin();

    mSceneManager.render();

    mGuiLayer.end();
}
