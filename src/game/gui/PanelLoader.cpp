#include "PanelLoader.h"

#include <imgui.h>

#include "../Engine.h"

#include "panels/LoginPanel.h"

using namespace lakot;

void PanelLoader::load(GuiLayer& pGuiLayer, PanelContext pPanelContext)
{
    switch (pPanelContext)
    {
        case PanelContext::Login:
        {
            loadLoginPanels(pGuiLayer);
            break;
        }

        case PanelContext::World:
        {
            loadWorldPanels(pGuiLayer);
            break;
        }
        default:
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "PanelLoader: Unknown context!");
            break;
        }
    }
}

void PanelLoader::loadLoginPanels(GuiLayer& pGuiLayer)
{
    Engine& tEngine = Engine::getInstance();

    pGuiLayer.addPanel(std::make_shared<LoginPanel>());
}

void PanelLoader::loadWorldPanels(GuiLayer& pGuiLayer)
{

}
