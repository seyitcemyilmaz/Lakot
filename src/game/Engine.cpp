#include "Engine.h"
#include "graphics/model/MotionLibrary.h"

#include "scene/LoginScene.h"

#if defined(LAKOT_DEV_MODE)
#include "scene/WorldScene.h"
#endif


using namespace lakot;

Engine& Engine::getInstance()
{
    static Engine tInstance;
    return tInstance;
}

bool Engine::initialize()
{
    SDL_Log("Engine is initializing.");

    std::string tMapError;

    if (!mMapCatalog.load(std::string(SDL_GetBasePath()) + "data", tMapError))
    {
        SDL_Log("Map data could not be loaded: %s", tMapError.c_str());
        return false;
    }

    if (!mMonsterCatalog.load(std::string(SDL_GetBasePath()) + "data", tMapError))
    {
        SDL_Log("Monster data could not be loaded: %s", tMapError.c_str());
        return false;
    }

    mDisplaySettings.load(mWindow);
    mDisplaySettings.apply(mWindow);

    mLastTime = SDL_GetPerformanceCounter();

    mNetworkManager.setConnectionStateCallback(
    [this](ConnectionStateType pState)
    {
        onConnectionStateChanged(pState);
    });

    mNetworkManager.start();

    mShaderManager.initialize();

    mAssetManager = std::make_unique<AssetManager>(std::string(SDL_GetBasePath()) + "data");
    mMotionLibrary = std::make_unique<MotionLibrary>(*mAssetManager);

    if (!mMotionLibrary->load("motions.json", tMapError))
    {
        SDL_Log("Motion data could not be loaded: %s", tMapError.c_str());
        return false;
    }

    mGuiLayer.initialize(mWindow, mGLContext);
    mRmlUiLayer.initialize(mWindow, mGLContext);

#if defined(LAKOT_DEV_MODE)
    mSceneManager.setNextScene(std::make_unique<WorldScene>(*this));
#else
    mSceneManager.setNextScene(std::make_unique<LoginScene>(*this));
#endif

    SDL_Log("Engine is initialized.");

    return true;
}

void Engine::deinitialize()
{
    // Before Rml::Shutdown() below - scenes and the overlay own RmlUi
    // documents whose cleanup must still find RmlUi alive.
    mSceneManager.shutdown();
    mMotionLibrary.reset();
    mAssetManager.reset();
    mReconnectOverlay.reset();

    mNetworkManager.stop();

    mShaderManager.deinitialize();
    mRmlUiLayer.deinitialize();
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

    // The scene is frozen while reconnecting: nothing it would do (moving,
    // sending state) means anything until the session is back.
    bool tIsSceneFrozen = mNetworkManager.isReconnecting();

    while (mAccumulator >= mFixedDeltaTime)
    {
        if (!tIsSceneFrozen)
        {
            mSceneManager.update(mFixedDeltaTime);
        }

        mAccumulator -= mFixedDeltaTime;
    }

    if (mReconnectOverlay)
    {
        if (tIsSceneFrozen)
        {
            mReconnectOverlay->setSecondsLeft(mNetworkManager.getReconnectSecondsLeft());
        }
        else
        {
            mReconnectOverlay.reset();
        }
    }

    mGarbageCollector.executeSynchronousTasks();

    draw();
}

double Engine::getInterpolationAlpha() const
{
    return mAccumulator / mFixedDeltaTime;
}

double Engine::getFixedDeltaTime() const
{
    return mFixedDeltaTime;
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

RmlUiLayer& Engine::getRmlUiLayer()
{
    return mRmlUiLayer;
}

const MapCatalog& Engine::getMapCatalog() const
{
    return mMapCatalog;
}

const MonsterCatalog& Engine::getMonsterCatalog() const
{
    return mMonsterCatalog;
}

void Engine::setClearColor(const glm::vec3& pColor)
{
    mClearColor = pColor;
}

MotionLibrary& Engine::getMotionLibrary()
{
    return *mMotionLibrary;
}

AssetManager& Engine::getAssetManager()
{
    return *mAssetManager;
}

DisplaySettings& Engine::getDisplaySettings()
{
    return mDisplaySettings;
}

SDL_AppResult Engine::eventHandler(SDL_Event* pEvent)
{
    if (pEvent->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    SDL_Window* tEventWindow = SDL_GetWindowFromEvent(pEvent);

    if (tEventWindow && tEventWindow != mWindow)
    {
        mGuiLayer.handleEvent(pEvent);
        return SDL_APP_CONTINUE;
    }

    if (pEvent->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        if (!mNetworkManager.isReconnecting() && mSceneManager.onCloseRequested())
        {
            return SDL_APP_CONTINUE;
        }

        return SDL_APP_SUCCESS;
    }

    mGuiLayer.handleEvent(pEvent);

    bool tIsMouseEvent = pEvent->type == SDL_EVENT_MOUSE_MOTION
                         || pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN
                         || pEvent->type == SDL_EVENT_MOUSE_BUTTON_UP
                         || pEvent->type == SDL_EVENT_MOUSE_WHEEL;

    // A scene holding the mouse gets it first and alone - see
    // Scene::isCapturingMouse(). Not while reconnecting: the scene is frozen
    // then and the overlay's buttons must stay clickable.
    if (tIsMouseEvent && !mNetworkManager.isReconnecting() && mSceneManager.isCapturingMouse())
    {
        mSceneManager.handleEvent(pEvent);
        return SDL_APP_CONTINUE;
    }

    if (mRmlUiLayer.handleEvent(pEvent))
    {
        return SDL_APP_CONTINUE;
    }

    if (!mNetworkManager.isReconnecting())
    {
        mSceneManager.handleEvent(pEvent);
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

void Engine::onConnectionStateChanged(ConnectionStateType pState)
{
    switch (pState)
    {
        case ConnectionStateType::eReconnecting:
        {
            // A drop mid camera-orbit would otherwise leave the cursor
            // captured, with no way to reach the overlay's button.
            SDL_SetWindowRelativeMouseMode(mWindow, false);

            mReconnectOverlay = std::make_unique<ReconnectRmlController>(mRmlUiLayer);
            mReconnectOverlay->setSecondsLeft(mNetworkManager.getReconnectSecondsLeft());

            mReconnectOverlay->setLogoutCallback(
            [this]()
            {
                // The overlay itself is destroyed by render() - not here,
                // inside its own event handler.
                mNetworkManager.stopReconnecting();
                mSceneManager.setNextScene(std::make_unique<LoginScene>(*this));
            });

            break;
        }
        case ConnectionStateType::eResumed:
        {
            mReconnectOverlay.reset();
            mSceneManager.onSessionResumed();
            break;
        }
        case ConnectionStateType::eLost:
        {
            mReconnectOverlay.reset();
            mSceneManager.setNextScene(std::make_unique<LoginScene>(*this, "Connection to the server was lost."));
            break;
        }
    }
}

void Engine::draw()
{
    int tWidth;
    int tHeight;

    SDL_GetWindowSizeInPixels(mWindow, &tWidth, &tHeight);
    glViewport(0, 0, tWidth, tHeight);

    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mGuiLayer.begin();

    mSceneManager.render();

    mGuiLayer.end();

    // RmlUi (player-facing UI) draws on top of ImGui (dev/debug tooling
    // only) - matching the hybrid's intent that debug tools sit underneath
    // the real UI.
    mRmlUiLayer.update();
    mRmlUiLayer.render();
}
