#ifndef LAKOT_PANEL_H
#define LAKOT_PANEL_H

#include <string>

#include <imgui.h>

namespace lakot
{

struct Panel
{
    std::string name;

    bool isOpen;

    ImGuiWindowFlags windowFlags;
    bool centerWindow;
    bool isFullScreen;

    virtual ~Panel()
    {

    }

    Panel(const std::string& pName, ImGuiWindowFlags pFlags = 0, bool pCenter = false, bool pFullScreen = false)
        : name(pName)
        , isOpen(true)
        , windowFlags(pFlags)
        , centerWindow(pCenter)
        , isFullScreen(pFullScreen)
    {

    }

    virtual void render() = 0;
};

}

#endif
