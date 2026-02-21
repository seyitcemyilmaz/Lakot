#include "LoginScene.h"

#include "../gui/PanelLoader.h"
#include "../Engine.h"

using namespace lakot;

LoginScene::~LoginScene()
{

}

LoginScene::LoginScene(Engine& pEngine)
    : Scene(pEngine)
{

}

void LoginScene::enter()
{
    SDL_Log("LoginScene: enter");

    GuiLayer& tGui = mEngine.getGuiLayer();
    tGui.clearPanels();
    PanelLoader::load(tGui, PanelContext::Login);
}

void LoginScene::exit()
{
    SDL_Log("LoginScene: exit");
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
