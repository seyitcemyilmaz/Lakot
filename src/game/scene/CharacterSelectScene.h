#ifndef LAKOT_CHARACTERSELECTSCENE_H
#define LAKOT_CHARACTERSELECTSCENE_H

#include <memory>

#include "Scene.h"

#include "../gui/rml/CharacterSelectRmlController.h"

namespace lakot
{

// Sits between LoginScene and WorldScene. Logging in authenticates an
// account; this is where one of that account's characters is chosen (or
// created), and only entering the world from here puts anything into a zone.
//
// The scene owns the networking side of that flow - the controller it holds
// is presentation only - because deciding when to change scenes is a scene's
// job, not a UI widget's.
class CharacterSelectScene final : public Scene
{
public:
    virtual ~CharacterSelectScene() override;
    explicit CharacterSelectScene(Engine& pEngine);

    void enter() override;
    void exit() override;
    void update(double pDeltaTime) override;
    void render() override;
    bool handleEvent(SDL_Event* pEvent) override;

private:
    std::unique_ptr<CharacterSelectRmlController> mController;
};

}

#endif
