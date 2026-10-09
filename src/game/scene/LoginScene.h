#ifndef LAKOT_LOGINSCENE_H
#define LAKOT_LOGINSCENE_H

#include <memory>
#include <string>

#include "Scene.h"

#include "../gui/rml/LoginRmlController.h"

namespace lakot
{

class LoginScene final : public Scene
{
public:
    virtual ~LoginScene() override;
    // pNotice is shown as an error on arrival - e.g. why the player was sent
    // back here.
    explicit LoginScene(Engine& pEngine, std::string pNotice = {});

    void enter() override;

    void exit() override;

    void update(double pDeltaTime) override;

    void render() override;

    bool handleEvent(SDL_Event* pEvent) override;

private:
    // Owns the RmlUi login document for the lifetime of this scene - created
    // in enter(), torn down in exit() (also unloads its document/data model,
    // see LoginRmlController's destructor).
    std::unique_ptr<LoginRmlController> mLoginController;

    std::string mNotice;
};

}

#endif
