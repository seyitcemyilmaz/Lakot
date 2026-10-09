#include "PanelLoader.h"

#include <imgui.h>

#include "../Engine.h"

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
    // The login screen is now an RmlUi document (LoginRmlController, loaded
    // directly by LoginScene::enter()) rather than an ImGui Panel - this
    // context is kept for whatever ImGui dev/debug panels get added later,
    // same as PanelContext::World below.
    (void)pGuiLayer;
}

void PanelLoader::loadWorldPanels(GuiLayer& pGuiLayer)
{

}
