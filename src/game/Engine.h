#ifndef LAKOT_ENGINE_H
#define LAKOT_ENGINE_H

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include <string>

#include <glm/glm.hpp>

#include "GarbageCollector.h"
#include "MapCatalog.h"
#include "MonsterCatalog.h"

#include "asset/AssetManager.h"
#include "network/NetworkManager.h"
#include "scene/SceneManager.h"
#include "gui/GuiLayer.h"
#include "gui/rml/RmlUiLayer.h"
#include "gui/rml/ReconnectRmlController.h"
#include "graphics/render/ShaderManager.h"
#include "settings/DisplaySettings.h"

namespace lakot
{

class MotionLibrary;

class Engine
{
public:
    static Engine& getInstance();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    // false if something the game cannot run without failed to load.
    bool initialize();
    void deinitialize();

    void render();

    // How far the current frame sits between the last fixed update and the
    // next one, 0..1 - for drawing moving things between their update steps.
    double getInterpolationAlpha() const;

    double getFixedDeltaTime() const;

    SDL_Window* getWindow() const;
    void setWindow(SDL_Window* pWindow);

    SDL_GLContext getGLContext() const;
    void setGLContext(SDL_GLContext pGLContext);

    NetworkManager& getNetworkManager();

    SceneManager& getSceneManager();
    ShaderManager& getShaderManager();

    GuiLayer& getGuiLayer();
    RmlUiLayer& getRmlUiLayer();

    // Maps and kingdoms from data/, loaded once at startup.
    const MapCatalog& getMapCatalog() const;
    const MonsterCatalog& getMonsterCatalog() const;

    void setClearColor(const glm::vec3& pColor);

    DisplaySettings& getDisplaySettings();

    AssetManager& getAssetManager();
    MotionLibrary& getMotionLibrary();

    SDL_AppResult eventHandler(SDL_Event* pEvent);

protected:
    friend class PanelLoader;

private:
    Engine();
    ~Engine();

    SDL_Window* mWindow;
    SDL_GLContext mGLContext;

    GarbageCollector mGarbageCollector;

    MapCatalog mMapCatalog;
    MonsterCatalog mMonsterCatalog;

    DisplaySettings mDisplaySettings;

    std::unique_ptr<AssetManager> mAssetManager;
    std::unique_ptr<MotionLibrary> mMotionLibrary;

    glm::vec3 mClearColor{0.4f, 0.4f, 0.4f};

    NetworkManager mNetworkManager;

    SceneManager mSceneManager;
    ShaderManager mShaderManager;

    GuiLayer mGuiLayer;
    RmlUiLayer mRmlUiLayer;

    // Exists only while NetworkManager is reconnecting.
    std::unique_ptr<ReconnectRmlController> mReconnectOverlay;

    uint64_t mLastTime;

    double mAccumulator;
    double mFrameTime;

    double mFixedDeltaTime;

    void draw();

    void onConnectionStateChanged(ConnectionStateType pState);
};

}

#endif
