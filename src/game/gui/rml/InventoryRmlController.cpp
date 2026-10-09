#include "InventoryRmlController.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "RmlUiLayer.h"

#include "../../network/controller/InventoryController.h"

#include "../../item/ItemIconRenderer.h"

using namespace lakot;

namespace
{
    Rml::String toPixels(int pValue)
    {
        return Rml::String(std::to_string(pValue) + "px");
    }

    Rml::String toDp(uint32_t pValue)
    {
        return Rml::String(std::to_string(pValue) + "dp");
    }

    constexpr int kRightMouseButton = 1;
    constexpr uint64_t kStatusDurationMs = 3000;
    constexpr float kScreenMargin = 4.0f;

    uint32_t getFootprintWidth(const ItemTemplate* pTemplate)
    {
        return pTemplate && pTemplate->width > 0 ? pTemplate->width : 1;
    }

    uint32_t getFootprintHeight(const ItemTemplate* pTemplate)
    {
        return pTemplate && pTemplate->height > 0 ? pTemplate->height : 1;
    }

    bool isEquippable(const ItemTemplate* pTemplate)
    {
        return pTemplate && pTemplate->equipSlot != EquipSlotType::eNone;
    }
}

const InventoryRmlController::DollSlotPosition InventoryRmlController::kDollSlots[] =
{
    { EquipSlotType::eHelmet, 61,   8, 2, 2 },
    { EquipSlotType::eArmor,  61,  80, 2, 2 },
    { EquipSlotType::eBoots,  61, 152, 2, 1 },
    { EquipSlotType::eWeapon, 17,  72, 1, 3 },
};

const size_t InventoryRmlController::kDollSlotCount =
    sizeof(InventoryRmlController::kDollSlots) / sizeof(InventoryRmlController::kDollSlots[0]);

const char* InventoryRmlController::getSlotRoleName(EquipSlotType pSlot)
{
    switch (pSlot)
    {
        case EquipSlotType::eWeapon: return "Weapon";
        case EquipSlotType::eArmor:  return "Armor";
        case EquipSlotType::eHelmet: return "Helmet";
        case EquipSlotType::eBoots:  return "Boots";
        default:                     return "";
    }
}

Rml::String InventoryRmlController::toIconPath(const OwnedItem& pItem, const ItemTemplate* pTemplate) const
{
    if (!pTemplate)
    {
        return Rml::String();
    }

    if (!pTemplate->model.empty())
    {
        return Rml::String(mItemIcons.getIconSource({ pItem.templateId, 0 }));
    }

    return pTemplate->icon.empty() ? Rml::String() : Rml::String("textures/items/" + pTemplate->icon);
}

InventoryRmlController::InventoryRmlController(RmlUiLayer& pRmlUiLayer, const InventoryController& pInventoryController, ItemIconRenderer& pItemIcons)
    : mRmlUiLayer(pRmlUiLayer)
    , mInventoryController(pInventoryController)
    , mItemIcons(pItemIcons)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("inventory");

    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tCell = tConstructor.RegisterStruct<CellRow>();
        tCell.RegisterMember("left", &CellRow::left);
        tCell.RegisterMember("top", &CellRow::top);
        tConstructor.RegisterArray<std::vector<CellRow>>();

        auto tPiece = tConstructor.RegisterStruct<PieceRow>();
        tPiece.RegisterMember("slot", &PieceRow::slot);
        tPiece.RegisterMember("left", &PieceRow::left);
        tPiece.RegisterMember("top", &PieceRow::top);
        tPiece.RegisterMember("width", &PieceRow::width);
        tPiece.RegisterMember("height", &PieceRow::height);
        tPiece.RegisterMember("icon", &PieceRow::icon);
        tPiece.RegisterMember("count", &PieceRow::count);
        tPiece.RegisterMember("has_count", &PieceRow::hasCount);
        tPiece.RegisterMember("is_dragged", &PieceRow::isDragged);
        tConstructor.RegisterArray<std::vector<PieceRow>>();

        auto tPage = tConstructor.RegisterStruct<PageRow>();
        tPage.RegisterMember("index", &PageRow::index);
        tPage.RegisterMember("label", &PageRow::label);
        tPage.RegisterMember("is_active", &PageRow::isActive);
        tConstructor.RegisterArray<std::vector<PageRow>>();

        auto tEquip = tConstructor.RegisterStruct<EquipRow>();
        tEquip.RegisterMember("index", &EquipRow::index);
        tEquip.RegisterMember("icon", &EquipRow::icon);
        tEquip.RegisterMember("left", &EquipRow::left);
        tEquip.RegisterMember("top", &EquipRow::top);
        tEquip.RegisterMember("width", &EquipRow::width);
        tEquip.RegisterMember("height", &EquipRow::height);
        tEquip.RegisterMember("item_left", &EquipRow::itemLeft);
        tEquip.RegisterMember("item_top", &EquipRow::itemTop);
        tEquip.RegisterMember("item_width", &EquipRow::itemWidth);
        tEquip.RegisterMember("item_height", &EquipRow::itemHeight);
        tEquip.RegisterMember("is_filled", &EquipRow::isFilled);
        tEquip.RegisterMember("is_dragged", &EquipRow::isDragged);
        tConstructor.RegisterArray<std::vector<EquipRow>>();

        auto tLine = tConstructor.RegisterStruct<TooltipLine>();
        tLine.RegisterMember("text", &TooltipLine::text);
        tConstructor.RegisterArray<std::vector<TooltipLine>>();

        sTypesRegistered = true;
    }

    tConstructor.Bind("cells", &mCells);
    tConstructor.Bind("pieces", &mPieces);
    tConstructor.Bind("pages", &mPages);
    tConstructor.Bind("equipment", &mEquipment);

    tConstructor.Bind("grid_width_px", &mGridWidthPx);
    tConstructor.Bind("grid_height_px", &mGridHeightPx);

    tConstructor.Bind("tooltip_lines", &mTooltipLines);
    tConstructor.Bind("tooltip_name", &mTooltipName);
    tConstructor.Bind("tooltip_hint", &mTooltipHint);
    tConstructor.Bind("tooltip_left", &mTooltipLeft);
    tConstructor.Bind("tooltip_top", &mTooltipTop);
    tConstructor.Bind("has_tooltip", &mHasTooltip);

    tConstructor.Bind("status_text", &mStatusText);
    tConstructor.Bind("has_status", &mHasStatusMessage);

    tConstructor.Bind("drag_ghost_icon", &mDragGhostIcon);
    tConstructor.Bind("drag_ghost_left", &mDragGhostLeft);
    tConstructor.Bind("drag_ghost_top", &mDragGhostTop);
    tConstructor.Bind("drag_ghost_width", &mDragGhostWidth);
    tConstructor.Bind("drag_ghost_height", &mDragGhostHeight);
    tConstructor.Bind("has_drag_ghost", &mHasDragGhost);

    tConstructor.BindEventCallback("piece_pressed", &InventoryRmlController::onPiecePressed, this);
    tConstructor.BindEventCallback("equip_slot_pressed", &InventoryRmlController::onEquipSlotPressed, this);
    tConstructor.BindEventCallback("hover_piece", &InventoryRmlController::onHoverPiece, this);
    tConstructor.BindEventCallback("hover_equipped", &InventoryRmlController::onHoverEquipped, this);
    tConstructor.BindEventCallback("unhover", &InventoryRmlController::onUnhover, this);
    tConstructor.BindEventCallback("set_page", &InventoryRmlController::onSetPageClicked, this);
    tConstructor.BindEventCallback("page_hovered", &InventoryRmlController::onPageHovered, this);
    tConstructor.BindEventCallback("close", &InventoryRmlController::onCloseClicked, this);
    tConstructor.BindEventCallback("drag_start", &InventoryRmlController::onDragStart, this);

    mModelHandle = tConstructor.GetModelHandle();

    rebuild();

    mDocument = mRmlUiLayer.loadDocument("ui/inventory.rml");
}

InventoryRmlController::~InventoryRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("inventory");
    }
}

void InventoryRmlController::setEquipRequestCallback(EquipRequestCallback pCallback)
{
    mEquipRequestCallback = pCallback;
}

void InventoryRmlController::setUnequipRequestCallback(UnequipRequestCallback pCallback)
{
    mUnequipRequestCallback = pCallback;
}

void InventoryRmlController::setMoveRequestCallback(MoveRequestCallback pCallback)
{
    mMoveRequestCallback = pCallback;
}

void InventoryRmlController::setContents(const std::vector<OwnedItem>& pItems, const CharacterStats& /*pStats*/)
{
    mOwnedItems = pItems;

    rebuild();
}

void InventoryRmlController::rebuild()
{
    const InventoryGrid& tGrid = mInventoryController.getGrid();

    uint32_t tCellsPerPage = tGrid.getCellsPerPage();

    if (mCurrentPage >= tGrid.pageCount)
    {
        mCurrentPage = 0;
    }

    mGridWidthPx = toDp(tGrid.width * kCellSize);
    mGridHeightPx = toDp(tGrid.pageHeight * kCellSize);

    mCells.clear();
    mCells.reserve(tCellsPerPage);

    for (uint32_t tY = 0; tY < tGrid.pageHeight; ++tY)
    {
        for (uint32_t tX = 0; tX < tGrid.width; ++tX)
        {
            mCells.push_back({ toDp(tX * kCellSize), toDp(tY * kCellSize) });
        }
    }

    mPages.clear();

    for (uint32_t tPage = 0; tPage < tGrid.pageCount; ++tPage)
    {
        PageRow tRow;
        tRow.index = static_cast<int>(tPage);
        tRow.label = std::to_string(tPage + 1);
        tRow.isActive = (tPage == mCurrentPage);

        mPages.push_back(std::move(tRow));
    }

    mEquipment.clear();

    for (size_t tIndex = 0; tIndex < kDollSlotCount; ++tIndex)
    {
        const DollSlotPosition& tPosition = kDollSlots[tIndex];

        EquipRow tRow;
        tRow.index = static_cast<int>(tPosition.slot);
        tRow.left = toDp(tPosition.left);
        tRow.top = toDp(tPosition.top);
        tRow.width = toDp(tPosition.columns * kCellSize);
        tRow.height = toDp(tPosition.rows * kCellSize);
        tRow.isFilled = false;

        mEquipment.push_back(std::move(tRow));
    }

    mPieces.clear();

    for (const OwnedItem& tItem : mOwnedItems)
    {
        const ItemTemplate* tTemplate = mInventoryController.findTemplate(tItem.templateId);

        if (tItem.location == ItemLocationType::eEquipped)
        {
            for (EquipRow& tRow : mEquipment)
            {
                if (static_cast<uint32_t>(tRow.index) != tItem.slot)
                {
                    continue;
                }

                const DollSlotPosition& tPosition = kDollSlots[&tRow - mEquipment.data()];

                uint32_t tColumns = std::min(getFootprintWidth(tTemplate), tPosition.columns);
                uint32_t tRows = std::min(getFootprintHeight(tTemplate), tPosition.rows);

                tRow.icon = toIconPath(tItem, tTemplate);
                tRow.itemLeft = toDp((tPosition.columns - tColumns) * kCellSize / 2);
                tRow.itemTop = toDp((tPosition.rows - tRows) * kCellSize / 2);
                tRow.itemWidth = toDp(tColumns * kCellSize);
                tRow.itemHeight = toDp(tRows * kCellSize);
                tRow.isFilled = true;
                tRow.isDragged = mIsDraggingItem && mDraggedFromEquip && tItem.slot == mDraggedFromSlot;
            }

            continue;
        }

        if (tItem.slot / tCellsPerPage != mCurrentPage)
        {
            continue;
        }

        uint32_t tWithinPage = tItem.slot % tCellsPerPage;
        uint32_t tX = tWithinPage % tGrid.width;
        uint32_t tY = tWithinPage / tGrid.width;

        PieceRow tRow;
        tRow.slot = static_cast<int>(tItem.slot);
        tRow.left = toDp(tX * kCellSize);
        tRow.top = toDp(tY * kCellSize);
        tRow.width = toDp(getFootprintWidth(tTemplate) * kCellSize);
        tRow.height = toDp(getFootprintHeight(tTemplate) * kCellSize);
        tRow.icon = toIconPath(tItem, tTemplate);
        tRow.count = std::to_string(tItem.count);
        tRow.hasCount = (tItem.count > 1);
        tRow.isDragged = mIsDraggingItem && !mDraggedFromEquip && tItem.slot == mDraggedFromSlot;

        mPieces.push_back(std::move(tRow));
    }

    dirtyAll();
}

void InventoryRmlController::showTooltip(const ItemTemplate* pTemplate, uint32_t pCount, bool pIsEquipped)
{
    mTooltipLines.clear();

    if (!pTemplate)
    {
        mTooltipName = "Unknown item";
        mTooltipHint = "";
    }
    else
    {
        mTooltipName = pTemplate->name;

        if (pCount > 1)
        {
            mTooltipLines.push_back({ Rml::String("Quantity: " + std::to_string(pCount)) });
        }

        if (pTemplate->bonusAttack != 0)
            mTooltipLines.push_back({ Rml::String("+" + std::to_string(pTemplate->bonusAttack) + " Attack") });
        if (pTemplate->bonusDefense != 0)
            mTooltipLines.push_back({ Rml::String("+" + std::to_string(pTemplate->bonusDefense) + " Defense") });
        if (pTemplate->bonusMaxHealth != 0)
            mTooltipLines.push_back({ Rml::String("+" + std::to_string(pTemplate->bonusMaxHealth) + " Max Health") });
        if (pTemplate->bonusMaxMana != 0)
            mTooltipLines.push_back({ Rml::String("+" + std::to_string(pTemplate->bonusMaxMana) + " Max Mana") });

        if (pTemplate->requiredLevel > 1)
            mTooltipLines.push_back({ Rml::String("Requires level " + std::to_string(pTemplate->requiredLevel)) });

        if (pIsEquipped)
        {
            mTooltipHint = "Right-click to unequip";
        }
        else if (isEquippable(pTemplate))
        {
            mTooltipHint = "Right-click to equip";
        }
        else
        {
            mTooltipHint = "";
        }
    }

    mHasTooltip = true;
    dirtyAll();

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    SDL_GetMouseState(&tMouseX, &tMouseY);
    positionTooltip(tMouseX, tMouseY);
}

void InventoryRmlController::showEmptySlotTooltip(EquipSlotType pSlot)
{
    mTooltipLines.clear();
    mTooltipName = getSlotRoleName(pSlot);
    mTooltipHint = "Empty";

    mHasTooltip = true;
    dirtyAll();

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    SDL_GetMouseState(&tMouseX, &tMouseY);
    positionTooltip(tMouseX, tMouseY);
}

void InventoryRmlController::hideTooltip()
{
    if (!mHasTooltip)
    {
        return;
    }

    mHasTooltip = false;
    dirtyAll();
}

void InventoryRmlController::positionTooltip(float pMouseX, float pMouseY)
{
    if (!mDocument)
    {
        return;
    }

    Rml::Vector2i tScreen = mRmlUiLayer.getContext()->GetDimensions();

    float tWidth = 200.0f;
    float tHeight = 0.0f;

    if (Rml::Element* tTooltip = mDocument->QuerySelector(".inv-tooltip"))
    {
        tWidth = std::max(tWidth, tTooltip->GetOffsetWidth());
        tHeight = tTooltip->GetOffsetHeight();
    }

    float tLeft = pMouseX + 18.0f;
    float tTop = pMouseY + 8.0f;

    if (tLeft + tWidth > tScreen.x - kScreenMargin)
    {
        tLeft = pMouseX - 12.0f - tWidth;
    }

    tTop = std::min(tTop, tScreen.y - kScreenMargin - tHeight);
    tTop = std::max(tTop, kScreenMargin);
    tLeft = std::max(tLeft, kScreenMargin);

    Rml::String tNewLeft = toPixels(static_cast<int>(tLeft - mDocument->GetAbsoluteLeft()));
    Rml::String tNewTop = toPixels(static_cast<int>(tTop - mDocument->GetAbsoluteTop()));

    if (tNewLeft != mTooltipLeft || tNewTop != mTooltipTop)
    {
        mTooltipLeft = tNewLeft;
        mTooltipTop = tNewTop;
        mModelHandle.DirtyVariable("tooltip_left");
        mModelHandle.DirtyVariable("tooltip_top");
    }
}

void InventoryRmlController::onHoverPiece(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mIsDraggingItem)
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(-1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location == ItemLocationType::eInventory && static_cast<int>(tItem.slot) == tSlot)
        {
            showTooltip(mInventoryController.findTemplate(tItem.templateId), tItem.count, false);
            return;
        }
    }
}

void InventoryRmlController::onHoverEquipped(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mIsDraggingItem)
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(-1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location == ItemLocationType::eEquipped && static_cast<int>(tItem.slot) == tSlot)
        {
            showTooltip(mInventoryController.findTemplate(tItem.templateId), tItem.count, true);
            return;
        }
    }

    if (tSlot > 0 && tSlot < static_cast<int>(EquipSlotType::eCount))
    {
        showEmptySlotTooltip(static_cast<EquipSlotType>(tSlot));
    }
}

void InventoryRmlController::onUnhover(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    hideTooltip();
}

void InventoryRmlController::onPiecePressed(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mIsDraggingItem)
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(-1);
    int tButton = pEvent.GetParameter<int>("button", -1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location != ItemLocationType::eInventory || static_cast<int>(tItem.slot) != tSlot)
        {
            continue;
        }

        if (tButton == kRightMouseButton)
        {
            if (isEquippable(mInventoryController.findTemplate(tItem.templateId)) && mEquipRequestCallback)
            {
                hideTooltip();
                request(RequestType::eEquip);
                mEquipRequestCallback(tItem.slot);
            }
        }
        else if (tButton == 0)
        {
            startItemDrag(tItem, false, pEvent);
        }

        return;
    }
}

void InventoryRmlController::onEquipSlotPressed(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mIsDraggingItem)
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(0);
    int tButton = pEvent.GetParameter<int>("button", -1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location != ItemLocationType::eEquipped || static_cast<int>(tItem.slot) != tSlot)
        {
            continue;
        }

        if (tButton == kRightMouseButton)
        {
            if (mUnequipRequestCallback)
            {
                hideTooltip();
                request(RequestType::eUnequip);
                mUnequipRequestCallback(static_cast<EquipSlotType>(tSlot), std::nullopt);
            }
        }
        else if (tButton == 0)
        {
            startItemDrag(tItem, true, pEvent);
        }

        return;
    }
}

void InventoryRmlController::startItemDrag(const OwnedItem& pItem, bool pIsFromEquip, Rml::Event& pEvent)
{
    const ItemTemplate* tTemplate = mInventoryController.findTemplate(pItem.templateId);

    float tScale = mRmlUiLayer.getScale();
    float tGhostWidth = getFootprintWidth(tTemplate) * kCellSize * tScale;
    float tGhostHeight = getFootprintHeight(tTemplate) * kCellSize * tScale;

    mIsDraggingItem = true;
    mDraggedFromEquip = pIsFromEquip;
    mDraggedFromSlot = pItem.slot;
    mDragGhostIcon = toIconPath(pItem, tTemplate);
    mDragGhostWidth = toDp(getFootprintWidth(tTemplate) * kCellSize);
    mDragGhostHeight = toDp(getFootprintHeight(tTemplate) * kCellSize);
    mHasDragGhost = true;

    float tMouseX = static_cast<float>(pEvent.GetParameter<int>("mouse_x", 0));
    float tMouseY = static_cast<float>(pEvent.GetParameter<int>("mouse_y", 0));

    mDragGrabOffsetX = 0.0f;
    mDragGrabOffsetY = 0.0f;

    if (Rml::Element* tElement = pEvent.GetCurrentElement())
    {
        mDragGrabOffsetX = std::clamp(tMouseX - tElement->GetAbsoluteLeft(), 0.0f, tGhostWidth - 1.0f);
        mDragGrabOffsetY = std::clamp(tMouseY - tElement->GetAbsoluteTop(), 0.0f, tGhostHeight - 1.0f);
    }

    mDragGhostLeft = toPixels(static_cast<int>(tMouseX - mDragGrabOffsetX - mDocument->GetAbsoluteLeft()));
    mDragGhostTop = toPixels(static_cast<int>(tMouseY - mDragGrabOffsetY - mDocument->GetAbsoluteTop()));

    mHasTooltip = false;
    rebuild();
}

void InventoryRmlController::finishItemDrag(float pMouseX, float pMouseY)
{
    float tCell = kCellSize * mRmlUiLayer.getScale();
    float tAnchorX = pMouseX - mDragGrabOffsetX + tCell * 0.5f;
    float tAnchorY = pMouseY - mDragGrabOffsetY + tCell * 0.5f;

    Rml::Element* tGrid = mDocument->QuerySelector(".inv-grid");

    if (tGrid && isInside(".inv-grid", tAnchorX, tAnchorY))
    {
        const InventoryGrid& tGridInfo = mInventoryController.getGrid();

        uint32_t tColumn = static_cast<uint32_t>((tAnchorX - tGrid->GetAbsoluteLeft()) / tCell);
        uint32_t tRow = static_cast<uint32_t>((tAnchorY - tGrid->GetAbsoluteTop()) / tCell);

        if (tColumn < tGridInfo.width && tRow < tGridInfo.pageHeight)
        {
            uint32_t tTargetSlot = mCurrentPage * tGridInfo.getCellsPerPage() + tRow * tGridInfo.width + tColumn;

            if (mDraggedFromEquip)
            {
                if (mUnequipRequestCallback)
                {
                    request(RequestType::eUnequip);
                    mUnequipRequestCallback(static_cast<EquipSlotType>(mDraggedFromSlot), tTargetSlot);
                }
            }
            else if (tTargetSlot != mDraggedFromSlot && mMoveRequestCallback)
            {
                request(RequestType::eMove);
                mMoveRequestCallback(mDraggedFromSlot, tTargetSlot);
            }
        }
    }
    else if (!mDraggedFromEquip && isInside(".inv-doll", pMouseX, pMouseY) && mEquipRequestCallback)
    {
        for (const OwnedItem& tItem : mOwnedItems)
        {
            if (tItem.location == ItemLocationType::eInventory && tItem.slot == mDraggedFromSlot)
            {
                if (isEquippable(mInventoryController.findTemplate(tItem.templateId)))
                {
                    request(RequestType::eEquip);
                    mEquipRequestCallback(mDraggedFromSlot);
                }

                break;
            }
        }
    }

    cancelItemDrag();
}

void InventoryRmlController::cancelItemDrag()
{
    if (!mIsDraggingItem)
    {
        return;
    }

    mIsDraggingItem = false;
    mHasDragGhost = false;

    rebuild();
}

void InventoryRmlController::onSetPageClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (!pArguments.empty())
    {
        setPage(static_cast<uint32_t>(std::max(pArguments[0].Get<int>(0), 0)));
    }
}

void InventoryRmlController::onPageHovered(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (mIsDraggingItem && !pArguments.empty())
    {
        setPage(static_cast<uint32_t>(std::max(pArguments[0].Get<int>(0), 0)));
    }
}

void InventoryRmlController::setPage(uint32_t pPage)
{
    if (pPage >= mInventoryController.getGrid().pageCount || pPage == mCurrentPage)
    {
        return;
    }

    mCurrentPage = pPage;

    mHasTooltip = false;
    rebuild();
}

void InventoryRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void InventoryRmlController::onDragStart(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    int tButton = pEvent.GetParameter<int>("button", -1);

    if (tButton != 0 || !mDocument)
    {
        return;
    }

    mIsDragging = true;
    mHasBeenMoved = true;
    SDL_GetMouseState(&mDragStartMouseX, &mDragStartMouseY);
    mDragStartDocLeft = mDocument->GetAbsoluteLeft();
    mDragStartDocTop = mDocument->GetAbsoluteTop();
}

void InventoryRmlController::request(RequestType pType)
{
    mLastRequest = pType;
}

void InventoryRmlController::onActionResult(bool pIsSuccess)
{
    if (pIsSuccess)
    {
        mHasStatusMessage = false;
        mStatusText.clear();
    }
    else
    {
        switch (mLastRequest)
        {
            case RequestType::eEquip:   mStatusText = "You cannot equip that right now."; break;
            case RequestType::eUnequip: mStatusText = "There is no room for it there."; break;
            case RequestType::eMove:    mStatusText = "There is no room for it there."; break;
            default:                    mStatusText = "That did not work."; break;
        }

        mHasStatusMessage = true;
        mStatusHideTime = SDL_GetTicks() + kStatusDurationMs;
    }

    mLastRequest = RequestType::eNone;

    dirtyAll();
}

void InventoryRmlController::toggle()
{
    if (mIsOpen)
    {
        close();
        return;
    }

    mIsOpen = true;

    mHasStatusMessage = false;
    mStatusText.clear();
    mHasTooltip = false;

    rebuild();

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void InventoryRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;
    mIsDragging = false;
    mHasTooltip = false;

    cancelItemDrag();

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool InventoryRmlController::isOpen() const
{
    return mIsOpen;
}

void InventoryRmlController::update()
{
    if (!mDocument || !mIsOpen)
    {
        return;
    }

    float tMouseX = 0.0f;
    float tMouseY = 0.0f;
    bool tIsLeftHeld = (SDL_GetMouseState(&tMouseX, &tMouseY) & SDL_BUTTON_LMASK) != 0;

    if (mIsDragging && !tIsLeftHeld)
    {
        mIsDragging = false;
    }

    keepWindowOnScreen(tMouseX, tMouseY);

    if (mIsDraggingItem)
    {
        if (tIsLeftHeld)
        {
            Rml::String tLeft = toPixels(static_cast<int>(tMouseX - mDragGrabOffsetX - mDocument->GetAbsoluteLeft()));
            Rml::String tTop = toPixels(static_cast<int>(tMouseY - mDragGrabOffsetY - mDocument->GetAbsoluteTop()));

            if (tLeft != mDragGhostLeft || tTop != mDragGhostTop)
            {
                mDragGhostLeft = tLeft;
                mDragGhostTop = tTop;
                mModelHandle.DirtyVariable("drag_ghost_left");
                mModelHandle.DirtyVariable("drag_ghost_top");
            }
        }
        else
        {
            finishItemDrag(tMouseX, tMouseY);
        }
    }

    if (mHasTooltip)
    {
        positionTooltip(tMouseX, tMouseY);
    }

    if (mHasStatusMessage && SDL_GetTicks() >= mStatusHideTime)
    {
        mHasStatusMessage = false;
        mModelHandle.DirtyVariable("has_status");
    }
}

void InventoryRmlController::keepWindowOnScreen(float pMouseX, float pMouseY)
{
    Rml::Element* tWindow = mDocument->QuerySelector(".inventory-window");

    if (!tWindow)
    {
        return;
    }

    float tDocLeft = mDocument->GetAbsoluteLeft();
    float tDocTop = mDocument->GetAbsoluteTop();

    float tWantedLeft = mHasBeenMoved ? tDocLeft : 0.0f;
    float tWantedTop = mHasBeenMoved ? tDocTop : 0.0f;

    if (mIsDragging)
    {
        tWantedLeft = mDragStartDocLeft + (pMouseX - mDragStartMouseX);
        tWantedTop = mDragStartDocTop + (pMouseY - mDragStartMouseY);
    }

    Rml::Vector2f tClamped = mRmlUiLayer.clampDocumentToScreen(mDocument, tWindow, Rml::Vector2f(tWantedLeft, tWantedTop));
    float tLeft = tClamped.x;
    float tTop = tClamped.y;

    if (std::abs(tLeft - tDocLeft) >= 0.5f || std::abs(tTop - tDocTop) >= 0.5f)
    {
        mDocument->SetProperty("left", toPixels(static_cast<int>(std::lround(tLeft))));
        mDocument->SetProperty("top", toPixels(static_cast<int>(std::lround(tTop))));
    }
}

bool InventoryRmlController::isInside(const char* pSelector, float pX, float pY) const
{
    Rml::Element* tElement = mDocument ? mDocument->QuerySelector(pSelector) : nullptr;

    if (!tElement)
    {
        return false;
    }

    float tLeft = tElement->GetAbsoluteLeft();
    float tTop = tElement->GetAbsoluteTop();

    return pX >= tLeft && pY >= tTop && pX < tLeft + tElement->GetOffsetWidth() && pY < tTop + tElement->GetOffsetHeight();
}

void InventoryRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();
}
