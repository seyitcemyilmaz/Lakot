#ifndef LAKOT_CAMERAELEMENT_H
#define LAKOT_CAMERAELEMENT_H

#include "core/interface/IGuiElement.h"

class CameraElement : public IGuiElement
{
public:
    virtual ~CameraElement();
    CameraElement();

    void render(void* pObject) override;
};

#endif
