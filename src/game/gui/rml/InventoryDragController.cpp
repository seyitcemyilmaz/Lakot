#include "InventoryDragController.h"

#include <algorithm>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>

#include "InventoryUnits.h"
#include "RmlUiLayer.h"

using namespace lakot;

InventoryDragController::InventoryDragController(RmlUiLayer& pRmlUiLayer, const RmlDocument& pDocument, uint32_t pCellSize)
    : mRmlUiLayer(pRmlUiLayer)
    , mDocument(pDocument)
    , mCellSize(pCellSize)
{
}

void InventoryDragController::bind(Rml::DataModelConstructor& pConstructor)
{
    pConstructor.Bind("drag_ghost_icon", &mGhostIcon);
    pConstructor.Bind("drag_ghost_left", &mGhostLeft);
    pConstructor.Bind("drag_ghost_top", &mGhostTop);
    pConstructor.Bind("drag_ghost_width", &mGhostWidth);
    pConstructor.Bind("drag_ghost_height", &mGhostHeight);
    pConstructor.Bind("has_drag_ghost", &mHasGhost);

    mModelHandle = pConstructor.GetModelHandle();
}

void InventoryDragController::begin(uint32_t pFromSlot, bool pIsFromEquip, const Rml::String& pIcon, uint32_t pColumns, uint32_t pRows, Rml::Event& pEvent)
{
    float tScale = mRmlUiLayer.getScale();
    float tGhostWidth = pColumns * mCellSize * tScale;
    float tGhostHeight = pRows * mCellSize * tScale;

    mIsActive = true;
    mIsFromEquip = pIsFromEquip;
    mFromSlot = pFromSlot;
    mGhostIcon = pIcon;
    mGhostWidth = InventoryUnits::toDp(pColumns * mCellSize);
    mGhostHeight = InventoryUnits::toDp(pRows * mCellSize);
    mHasGhost = true;

    float tMouseX = static_cast<float>(pEvent.GetParameter<int>("mouse_x", 0));
    float tMouseY = static_cast<float>(pEvent.GetParameter<int>("mouse_y", 0));

    mGrabOffsetX = 0.0f;
    mGrabOffsetY = 0.0f;

    if (Rml::Element* tElement = pEvent.GetCurrentElement())
    {
        mGrabOffsetX = std::clamp(tMouseX - tElement->GetAbsoluteLeft(), 0.0f, tGhostWidth - 1.0f);
        mGrabOffsetY = std::clamp(tMouseY - tElement->GetAbsoluteTop(), 0.0f, tGhostHeight - 1.0f);
    }

    mGhostLeft = InventoryUnits::toPixels(static_cast<int>(tMouseX - mGrabOffsetX - mDocument->GetAbsoluteLeft()));
    mGhostTop = InventoryUnits::toPixels(static_cast<int>(tMouseY - mGrabOffsetY - mDocument->GetAbsoluteTop()));
}

void InventoryDragController::follow(float pMouseX, float pMouseY)
{
    Rml::String tLeft = InventoryUnits::toPixels(static_cast<int>(pMouseX - mGrabOffsetX - mDocument->GetAbsoluteLeft()));
    Rml::String tTop = InventoryUnits::toPixels(static_cast<int>(pMouseY - mGrabOffsetY - mDocument->GetAbsoluteTop()));

    if (tLeft != mGhostLeft || tTop != mGhostTop)
    {
        mGhostLeft = tLeft;
        mGhostTop = tTop;
        mModelHandle.DirtyVariable("drag_ghost_left");
        mModelHandle.DirtyVariable("drag_ghost_top");
    }
}

void InventoryDragController::end()
{
    mIsActive = false;
    mHasGhost = false;
}

bool InventoryDragController::isActive() const
{
    return mIsActive;
}

bool InventoryDragController::isFromEquip() const
{
    return mIsFromEquip;
}

uint32_t InventoryDragController::getFromSlot() const
{
    return mFromSlot;
}

float InventoryDragController::getGrabOffsetX() const
{
    return mGrabOffsetX;
}

float InventoryDragController::getGrabOffsetY() const
{
    return mGrabOffsetY;
}
