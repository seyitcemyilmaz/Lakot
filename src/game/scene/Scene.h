#ifndef LAKOT_SCENE_H
#define LAKOT_SCENE_H

#include <SDL3/SDL.h>

namespace lakot
{

class Engine;

class Scene
{
public:
    virtual ~Scene() = default;
    explicit Scene(Engine& pEngine)
        : mEngine(pEngine)
    {

    }

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    virtual void enter() = 0;

    virtual void exit() = 0;

    virtual void update(double pDeltaTime) = 0;

    virtual void render() = 0;

    virtual bool handleEvent(SDL_Event* pEvent) = 0;

protected:
    Engine& mEngine;
};

}

#endif
