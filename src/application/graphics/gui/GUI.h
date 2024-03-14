#ifndef LAKOT_GUI_H
#define LAKOT_GUI_H

#include "core/interface/IGuiElement.h"
#include "application/graphics/scene/Scene.h"

class GUI
{
public:
    virtual ~GUI();
    GUI();

    void initialize();
    void deinitialize();

    void render(Scene* pScene);

    void setVisibility(bool pVisibility);
    bool getVisibility();

    bool isMouseEventOnGUI();
    bool isKeyboardEventOnGUI();

    void addGuiElement(IGuiElement* pGuiElement);

private:
    bool mVisibility;

    std::vector<IGuiElement*> mGuiElements;

    void preRenderPhase();
    void postRenderPhase();
};

#endif
