#ifndef LAKOT_GUILAYER_H
#define LAKOT_GUILAYER_H

#include <vector>
#include <memory>

#include <SDL3/SDL.h>

#include "Panel.h"

namespace lakot
{

class GuiLayer
{
public:
    GuiLayer() = default;
    ~GuiLayer() = default;

    void initialize(SDL_Window* pWindow, SDL_GLContext pGLContext);
    void deinitialize();

    void begin();
    void end();

    bool handleEvent(SDL_Event* pEvent);

    void addPanel(std::shared_ptr<Panel> pPanel);

    void clearPanels();

private:
    std::vector<std::shared_ptr<Panel>> mPanels;

};

}

#endif
