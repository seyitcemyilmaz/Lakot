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

    void update(double pDeltaTime);

    void render();

    bool handleEvent(SDL_Event* pEvent);

private:
    std::unique_ptr<Scene> mCurrentScene;
    std::unique_ptr<Scene> mNextScene;

    void applySceneChange();
};

}

#endif
