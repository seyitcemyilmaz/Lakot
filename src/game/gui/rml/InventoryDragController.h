#ifndef LAKOT_INVENTORY_DRAG_CONTROLLER_H
#define LAKOT_INVENTORY_DRAG_CONTROLLER_H

#include <cstdint>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Types.h>

#include "RmlDocument.h"

namespace Rml
{
class DataModelConstructor;
}

namespace lakot
{

class RmlUiLayer;

class InventoryDragController
{
public:
    InventoryDragController(RmlUiLayer& pRmlUiLayer, const RmlDocument& pDocument, uint32_t pCellSize);

    InventoryDragController(const InventoryDragController&) = delete;
    InventoryDragController& operator=(const InventoryDragController&) = delete;

    void bind(Rml::DataModelConstructor& pConstructor);

    void begin(uint32_t pFromSlot, bool pIsFromEquip, const Rml::String& pIcon, uint32_t pColumns, uint32_t pRows, Rml::Event& pEvent);
    void follow(float pMouseX, float pMouseY);
    void end();

    bool isActive() const;
    bool isFromEquip() const;
    uint32_t getFromSlot() const;
    float getGrabOffsetX() const;
    float getGrabOffsetY() const;

private:
    RmlUiLayer& mRmlUiLayer;
    const RmlDocument& mDocument;
    uint32_t mCellSize;

    Rml::DataModelHandle mModelHandle;

    bool mIsActive{false};
    uint32_t mFromSlot{0};
    bool mIsFromEquip{false};

    float mGrabOffsetX{0.0f};
    float mGrabOffsetY{0.0f};

    Rml::String mGhostIcon;
    Rml::String mGhostLeft{"0px"};
    Rml::String mGhostTop{"0px"};
    Rml::String mGhostWidth{"0px"};
    Rml::String mGhostHeight{"0px"};
    bool mHasGhost{false};
};

}

#endif
