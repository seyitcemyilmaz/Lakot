#ifndef LAKOT_WORLDSCENE_H
#define LAKOT_WORLDSCENE_H

#include <memory>

#include "Scene.h"

#include "../graphics/camera/FPSCamera.h"

#include "../graphics/geometry/BoxContainer.h"
#include "../graphics/geometry/Terrain.h"

namespace lakot
{

class WorldScene final : public Scene
{
public:
    virtual ~WorldScene() override;
    WorldScene(Engine& pEngine);

    void enter() override;

    void exit() override;

    void update(double pDeltaTime) override;

    void render() override;

    bool handleEvent(SDL_Event* pEvent) override;

private:
    std::unique_ptr<BoxContainer> mBoxContainer;
    std::unique_ptr<Terrain> mTerrain;

    std::unique_ptr<FPSCamera> mCamera;

    bool mIsMovingForward{false};
    bool mIsMovingBackward{false};
    bool mIsMovingLeft{false};
    bool mIsMovingRight{false};
};

}

#endif
