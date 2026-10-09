#ifndef LAKOT_INVENTORY_RML_CONTROLLER_H
#define LAKOT_INVENTORY_RML_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "../../network/GameTypes.h"

namespace lakot
{

class RmlUiLayer;
class InventoryController;

class ItemIconRenderer;

class InventoryRmlController
{
public:
    using EquipRequestCallback = std::function<void(uint32_t pBagSlot)>;
    using UnequipRequestCallback = std::function<void(EquipSlotType pSlot, std::optional<uint32_t> pBagSlot)>;
    using MoveRequestCallback = std::function<void(uint32_t pFromSlot, uint32_t pToSlot)>;

    InventoryRmlController(RmlUiLayer& pRmlUiLayer, const InventoryController& pInventoryController, ItemIconRenderer& pItemIcons);
    ~InventoryRmlController();

    InventoryRmlController(const InventoryRmlController&) = delete;
    InventoryRmlController& operator=(const InventoryRmlController&) = delete;

    void setEquipRequestCallback(EquipRequestCallback pCallback);
    void setUnequipRequestCallback(UnequipRequestCallback pCallback);
    void setMoveRequestCallback(MoveRequestCallback pCallback);

    void setContents(const std::vector<OwnedItem>& pItems, const CharacterStats& pStats);

    void onActionResult(bool pIsSuccess);

    void toggle();
    void close();
    bool isOpen() const;

    void update();

private:
    // In dp; must match .inv-cell in theme.rcss.
    static constexpr uint32_t kCellSize = 32;

    enum class RequestType
    {
        eNone,
        eEquip,
        eUnequip,
        eMove
    };

    struct DollSlotPosition
    {
        EquipSlotType slot;
        uint32_t left;
        uint32_t top;
        uint32_t columns;
        uint32_t rows;
    };

    static const DollSlotPosition kDollSlots[];
    static const size_t kDollSlotCount;

    struct CellRow
    {
        Rml::String left;
        Rml::String top;
    };

    struct PieceRow
    {
        int slot = 0;

        Rml::String left;
        Rml::String top;
        Rml::String width;
        Rml::String height;

        Rml::String icon;
        Rml::String count;
        bool hasCount = false;
        bool isDragged = false;
    };

    struct PageRow
    {
        int index = 0;
        Rml::String label;
        bool isActive = false;
    };

    struct EquipRow
    {
        int index = 0;
        Rml::String icon;
        Rml::String left;
        Rml::String top;
        Rml::String width;
        Rml::String height;
        Rml::String itemLeft;
        Rml::String itemTop;
        Rml::String itemWidth;
        Rml::String itemHeight;
        bool isFilled = false;
        bool isDragged = false;
    };

    struct TooltipLine
    {
        Rml::String text;
    };

    RmlUiLayer& mRmlUiLayer;
    const InventoryController& mInventoryController;
    ItemIconRenderer& mItemIcons;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    std::vector<OwnedItem> mOwnedItems;

    std::vector<CellRow> mCells;
    std::vector<PieceRow> mPieces;
    std::vector<PageRow> mPages;
    std::vector<EquipRow> mEquipment;

    Rml::String mGridWidthPx{"0px"};
    Rml::String mGridHeightPx{"0px"};

    std::vector<TooltipLine> mTooltipLines;
    Rml::String mTooltipName;
    Rml::String mTooltipHint;
    Rml::String mTooltipLeft{"0px"};
    Rml::String mTooltipTop{"0px"};
    bool mHasTooltip{false};

    Rml::String mStatusText;
    bool mHasStatusMessage{false};
    uint64_t mStatusHideTime{0};
    RequestType mLastRequest{RequestType::eNone};
    bool mIsOpen{false};

    uint32_t mCurrentPage{0};

    bool mIsDragging{false};
    bool mHasBeenMoved{false};
    float mDragStartMouseX{0.0f};
    float mDragStartMouseY{0.0f};
    float mDragStartDocLeft{0.0f};
    float mDragStartDocTop{0.0f};

    bool mIsDraggingItem{false};
    uint32_t mDraggedFromSlot{0};
    bool mDraggedFromEquip{false};

    float mDragGrabOffsetX{0.0f};
    float mDragGrabOffsetY{0.0f};

    Rml::String mDragGhostIcon;
    Rml::String mDragGhostLeft{"0px"};
    Rml::String mDragGhostTop{"0px"};
    Rml::String mDragGhostWidth{"0px"};
    Rml::String mDragGhostHeight{"0px"};
    bool mHasDragGhost{false};

    EquipRequestCallback mEquipRequestCallback;
    UnequipRequestCallback mUnequipRequestCallback;
    MoveRequestCallback mMoveRequestCallback;

    void onPiecePressed(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onEquipSlotPressed(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onHoverPiece(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onHoverEquipped(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onUnhover(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSetPageClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDragStart(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onPageHovered(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void rebuild();

    void showTooltip(const ItemTemplate* pTemplate, uint32_t pCount, bool pIsEquipped);
    void showEmptySlotTooltip(EquipSlotType pSlot);
    void hideTooltip();

    void startItemDrag(const OwnedItem& pItem, bool pIsFromEquip, Rml::Event& pEvent);
    void finishItemDrag(float pMouseX, float pMouseY);
    void cancelItemDrag();

    void positionTooltip(float pMouseX, float pMouseY);
    void keepWindowOnScreen(float pMouseX, float pMouseY);

    void setPage(uint32_t pPage);
    void request(RequestType pType);

    bool isInside(const char* pSelector, float pX, float pY) const;

    void dirtyAll();

    Rml::String toIconPath(const OwnedItem& pItem, const ItemTemplate* pTemplate) const;

    static const char* getSlotRoleName(EquipSlotType pSlot);
};

}

#endif
