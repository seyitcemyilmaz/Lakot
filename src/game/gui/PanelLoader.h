#ifndef LAKOT_PANELLOADER_H
#define LAKOT_PANELLOADER_H

#include "GuiLayer.h"

namespace lakot
{

enum class PanelContext
{
    Login,
    World
};

class PanelLoader
{
public:
    static void load(GuiLayer& pGuiLayer, PanelContext pPanelContext);

private:
    ~PanelLoader() = default;
    PanelLoader() = default;

    static void loadLoginPanels(GuiLayer& pGuiLayer);
    static void loadWorldPanels(GuiLayer& pGuiLayer);
};

}

#endif
