#ifndef LAKOT_SCENEMANAGER_H
#define LAKOT_SCENEMANAGER_H


#include <memory>
#include <SDL3/SDL.h>
#include "Scene.h"

namespace lakot
{

class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    void setNextScene(std::unique_ptr<Scene> pScene);

    // Destroys the current scene right now, synchronously - called from
    // Engine::deinitialize() BEFORE RmlUiLayer::deinitialize() (Rml::Shutdown()).
    // Without this, mCurrentScene (e.g. WorldScene, owning ChatRmlController
    // instances with live Rml::ElementDocument*/DataModelHandle members) only
    // gets destroyed later, at Engine's own static destruction at process
    // exit - by then Rml::Shutdown() has already torn down the context, and
    // those destructors' RmlUi cleanup calls hit "Resource used ... after it
    // was shut down" (RmlUi's ControlledLifetimeResource assert).
    void shutdown();

    void update(double pDeltaTime);

    void render();

    bool handleEvent(SDL_Event* pEvent);

    void onSessionResumed();

    bool isCapturingMouse() const;
    bool onCloseRequested();

private:
    std::unique_ptr<Scene> mCurrentScene;
    std::unique_ptr<Scene> mNextScene;

    void applySceneChange();
};

}

#endif
