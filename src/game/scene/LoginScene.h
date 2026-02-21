#ifndef LAKOT_LOGINSCENE_H
#define LAKOT_LOGINSCENE_H

#include "Scene.h"

namespace lakot
{

class LoginScene final : public Scene
{
public:
    virtual ~LoginScene() override;
    LoginScene(Engine& pEngine);

    void enter() override;

    void exit() override;

    void update(double pDeltaTime) override;

    void render() override;

    bool handleEvent(SDL_Event* pEvent) override;
};

}

#endif
