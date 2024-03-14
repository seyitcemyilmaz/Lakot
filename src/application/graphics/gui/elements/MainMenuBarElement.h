#ifndef LAKOT_MAINMENUBAR_H
#define LAKOT_MAINMENUBAR_H

#include "core/interface/IGuiElement.h"

class MainMenuBarElement : public IGuiElement
{
public:
    virtual ~MainMenuBarElement();
    MainMenuBarElement();

    void render(void* pObject) override;
};

#endif
