#include "LoginScene.h"

#include "../gui/PanelLoader.h"
#include "../Engine.h"

using namespace lakot;

LoginScene::~LoginScene()
{

}

LoginScene::LoginScene(Engine& pEngine, std::string pNotice)
    : Scene(pEngine)
    , mNotice(std::move(pNotice))
{

}

void LoginScene::enter()
{
    SDL_Log("LoginScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::Login);

    mLoginController = std::make_unique<LoginRmlController>(mEngine.getRmlUiLayer(), mNotice);
}

void LoginScene::exit()
{
    SDL_Log("LoginScene: exit");

    // Before destroying the controller, not after - AuthController outlives
    // this scene (Engine owns it for the whole process) and its callbacks
    // capture mLoginController's `this`, so leaving them registered would
    // point them at freed memory the moment any further Login/Register
    // response arrived.
    mEngine.getNetworkManager().getAuthController().clearCallbacks();

    mLoginController.reset();
}

void LoginScene::update(double pDeltaTime)
{

}

void LoginScene::render()
{

}

bool LoginScene::handleEvent(SDL_Event* pEvent)
{
    return false;
}
