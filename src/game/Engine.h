#ifndef LAKOT_ENGINE_H
#define LAKOT_ENGINE_H

#include <glad/glad.h>
#include <SDL3/SDL.h>

#include "GarbageCollector.h"

#include "network/NetworkManager.h"
#include "scene/SceneManager.h"
#include "gui/GuiLayer.h"
#include "graphics/render/ShaderManager.h"

namespace lakot
{

class Engine
{
public:
    static Engine& getInstance();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    void initialize();
    void deinitialize();

    void render();

    SDL_Window* getWindow() const;
    void setWindow(SDL_Window* pWindow);

    SDL_GLContext getGLContext() const;
    void setGLContext(SDL_GLContext pGLContext);

    NetworkManager& getNetworkManager();

    SceneManager& getSceneManager();
    ShaderManager& getShaderManager();

    GuiLayer& getGuiLayer();

    SDL_AppResult eventHandler(SDL_Event* pEvent);

protected:
    friend class PanelLoader;

private:
    Engine();
    ~Engine();

    SDL_Window* mWindow;
    SDL_GLContext mGLContext;

    GarbageCollector mGarbageCollector;

    NetworkManager mNetworkManager;

    SceneManager mSceneManager;
    ShaderManager mShaderManager;

    GuiLayer mGuiLayer;

    uint64_t mLastTime;

    double mAccumulator;
    double mFrameTime;

    double mFixedDeltaTime;

    void draw();
};

}

#endif
