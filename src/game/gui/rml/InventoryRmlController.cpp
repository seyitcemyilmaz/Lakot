#include "InventoryRmlController.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "InventoryUnits.h"
#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

#include "../../network/controller/InventoryController.h"

#include "../../item/ItemIconPath.h"
#include "../../item/ItemIconRenderer.h"

using namespace lakot;
using InventoryUnits::getFootprintHeight;
using InventoryUnits::getFootprintWidth;
using InventoryUnits::toDp;
using InventoryUnits::toPixels;

namespace
{
    constexpr int kRightMouseButton = 1;
    constexpr uint64_t kStatusDurationMs = 3000;

    bool isEquippable(const ItemTemplate* pTemplate)
    {
        return pTemplate && pTemplate->equipSlot != EquipSlotType::eNone;
    }
}

const InventoryRmlController::DollSlotPosition InventoryRmlController::kDollSlots[] =
{
    { EquipSlotType::eHelmet, 6,   8, 2, 2 },
    { EquipSlotType::eArmor,  6,  76, 2, 2 },
    { EquipSlotType::eBoots,  6, 148, 2, 1 },
    { EquipSlotType::eWeapon, 154, 76, 1, 3 },
};

const InventoryRmlController::DollSlotPosition InventoryRmlController::kDollReserved[] =
{
    { EquipSlotType::eNone, 188, 76, 1, 3 },
};

const size_t InventoryRmlController::kDollSlotCount =
    sizeof(InventoryRmlController::kDollSlots) / sizeof(InventoryRmlController::kDollSlots[0]);

Rml::String InventoryRmlController::toIconPath(const ItemTemplate* pTemplate) const
{
    Rml::String tPath(ItemIconPath::resolve(mItemIcons, pTemplate));

    if (!tPath.empty())
    {
        return tPath;
    }

    switch (pTemplate ? pTemplate->equipSlot : EquipSlotType::eNone)
    {
        case EquipSlotType::eWeapon: return "textures/items/slot_weapon.tga";
        case EquipSlotType::eArmor:  return "textures/items/slot_armor.tga";
        case EquipSlotType::eHelmet: return "textures/items/slot_helmet.tga";
        case EquipSlotType::eBoots:  return "textures/items/slot_boots.tga";
        default:                     return "textures/items/slot_none.tga";
    }
}

InventoryRmlController::InventoryRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const InventoryController& pInventoryController, const ClassCatalog& pClassCatalog, ItemIconRenderer& pItemIcons)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mInventoryController(pInventoryController)
    , mClassCatalog(pClassCatalog)
    , mItemIcons(pItemIcons)
    , mTooltip(mRmlUiLayer, mLocalization, mClassCatalog, mDocument)
    , mDrag(mRmlUiLayer, mDocument, kCellSize)
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

        sTypesRegistered = true;
    }

    tConstructor.Bind("cells", &mCells);
    tConstructor.Bind("pieces", &mPieces);
    tConstructor.Bind("pages", &mPages);
    tConstructor.Bind("equipment", &mEquipment);

    tConstructor.Bind("grid_width_px", &mGridWidthPx);
    tConstructor.Bind("grid_height_px", &mGridHeightPx);
    tConstructor.Bind("doll_width_px", &mDollWidthPx);
    tConstructor.Bind("reserved_left", &mReservedLeft);
    tConstructor.Bind("reserved_top", &mReservedTop);
    tConstructor.Bind("reserved_width", &mReservedWidth);
    tConstructor.Bind("reserved_height", &mReservedHeight);
    tConstructor.Bind("doll_height_px", &mDollHeightPx);

    mTooltip.bind(tConstructor);

    tConstructor.Bind("status_text", &mStatusText);
    tConstructor.Bind("has_status", &mHasStatusMessage);

    mDrag.bind(tConstructor);

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

    mDocument.load(mRmlUiLayer, "ui/inventory.rml");
}

InventoryRmlController::~InventoryRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
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

void InventoryRmlController::setItemDroppedOutsideCallback(ItemDroppedOutsideCallback pCallback)
{
    mItemDroppedOutsideCallback = pCallback;
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
    mDollWidthPx = toDp(kDollWidth);
    mReservedLeft = toDp(kDollReserved[0].left);
    mReservedTop = toDp(kDollReserved[0].top);
    mReservedWidth = toDp(kDollReserved[0].columns * kCellSize);
    mReservedHeight = toDp(kDollReserved[0].rows * kCellSize);
    mDollHeightPx = toDp(kDollHeight);

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

                InventoryUnits::IconFit tFit = InventoryUnits::fitContain(tTemplate, tColumns * kCellSize, tRows * kCellSize);

                tRow.icon = toIconPath(tTemplate);
                tRow.itemLeft = toDp((tPosition.columns * kCellSize - tFit.width) / 2);
                tRow.itemTop = toDp((tPosition.rows * kCellSize - tFit.height) / 2);
                tRow.itemWidth = toDp(tFit.width);
                tRow.itemHeight = toDp(tFit.height);
                tRow.isFilled = true;
                tRow.isDragged = mDrag.isActive() && mDrag.isFromEquip() && tItem.slot == mDrag.getFromSlot();
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
        tRow.icon = toIconPath(tTemplate);
        tRow.count = std::to_string(tItem.count);
        tRow.hasCount = (tItem.count > 1);
        tRow.isDragged = mDrag.isActive() && !mDrag.isFromEquip() && tItem.slot == mDrag.getFromSlot();

        mPieces.push_back(std::move(tRow));
    }

    dirtyAll();
}

void InventoryRmlController::onHoverPiece(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mDrag.isActive())
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(-1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location == ItemLocationType::eInventory && static_cast<int>(tItem.slot) == tSlot)
        {
            mTooltip.showItem(mInventoryController.findTemplate(tItem.templateId), tItem.count, false);
            return;
        }
    }
}

void InventoryRmlController::onHoverEquipped(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mDrag.isActive())
    {
        return;
    }

    int tSlot = pArguments[0].Get<int>(-1);

    for (const OwnedItem& tItem : mOwnedItems)
    {
        if (tItem.location == ItemLocationType::eEquipped && static_cast<int>(tItem.slot) == tSlot)
        {
            mTooltip.showItem(mInventoryController.findTemplate(tItem.templateId), tItem.count, true);
            return;
        }
    }

    if (tSlot > 0 && tSlot < static_cast<int>(EquipSlotType::eCount))
    {
        mTooltip.showEmptySlot(static_cast<EquipSlotType>(tSlot));
    }
}

void InventoryRmlController::onUnhover(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mTooltip.hide();
}

void InventoryRmlController::onPiecePressed(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || mDrag.isActive())
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
                mTooltip.hide();
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
    if (pArguments.empty() || mDrag.isActive())
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
                mTooltip.hide();
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

    mDrag.begin(pItem.slot, pIsFromEquip, toIconPath(tTemplate), getFootprintWidth(tTemplate), getFootprintHeight(tTemplate), pEvent);

    mTooltip.forget();
    rebuild();
}

void InventoryRmlController::finishItemDrag(float pMouseX, float pMouseY)
{
    float tCell = kCellSize * mRmlUiLayer.getScale();
    float tAnchorX = pMouseX - mDrag.getGrabOffsetX() + tCell * 0.5f;
    float tAnchorY = pMouseY - mDrag.getGrabOffsetY() + tCell * 0.5f;

    Rml::Element* tGrid = mDocument->QuerySelector(".inv-grid");

    if (tGrid && isInside(".inv-grid", tAnchorX, tAnchorY))
    {
        const InventoryGrid& tGridInfo = mInventoryController.getGrid();

        uint32_t tColumn = static_cast<uint32_t>((tAnchorX - tGrid->GetAbsoluteLeft()) / tCell);
        uint32_t tRow = static_cast<uint32_t>((tAnchorY - tGrid->GetAbsoluteTop()) / tCell);

        if (tColumn < tGridInfo.width && tRow < tGridInfo.pageHeight)
        {
            uint32_t tTargetSlot = mCurrentPage * tGridInfo.getCellsPerPage() + tRow * tGridInfo.width + tColumn;

            if (mDrag.isFromEquip())
            {
                if (mUnequipRequestCallback)
                {
                    request(RequestType::eUnequip);
                    mUnequipRequestCallback(static_cast<EquipSlotType>(mDrag.getFromSlot()), tTargetSlot);
                }
            }
            else if (tTargetSlot != mDrag.getFromSlot() && mMoveRequestCallback)
            {
                request(RequestType::eMove);
                mMoveRequestCallback(mDrag.getFromSlot(), tTargetSlot);
            }
        }
    }
    else if (!mDrag.isFromEquip() && !isInside(".inventory-window", pMouseX, pMouseY) && mItemDroppedOutsideCallback)
    {
        for (const OwnedItem& tItem : mOwnedItems)
        {
            if (tItem.location != ItemLocationType::eInventory || tItem.slot != mDrag.getFromSlot())
            {
                continue;
            }

            mItemDroppedOutsideCallback(tItem, pMouseX, pMouseY);

            break;
        }
    }
    else if (!mDrag.isFromEquip() && isInside(".inv-doll", pMouseX, pMouseY) && mEquipRequestCallback)
    {
        for (const OwnedItem& tItem : mOwnedItems)
        {
            if (tItem.location == ItemLocationType::eInventory && tItem.slot == mDrag.getFromSlot())
            {
                if (isEquippable(mInventoryController.findTemplate(tItem.templateId)))
                {
                    request(RequestType::eEquip);
                    mEquipRequestCallback(mDrag.getFromSlot());
                }

                break;
            }
        }
    }

    cancelItemDrag();
}

void InventoryRmlController::cancelItemDrag()
{
    if (!mDrag.isActive())
    {
        return;
    }

    mDrag.end();

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
    if (mDrag.isActive() && !pArguments.empty())
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

    mTooltip.forget();
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
            case RequestType::eEquip:   mStatusText = mLocalization.text("inventory.status.cannot_equip"); break;
            case RequestType::eUnequip: mStatusText = mLocalization.text("inventory.status.no_room"); break;
            case RequestType::eMove:    mStatusText = mLocalization.text("inventory.status.no_room"); break;
            default:                    mStatusText = mLocalization.text("inventory.status.failed"); break;
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
    mTooltip.forget();

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
    mTooltip.forget();

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

    if (mDrag.isActive())
    {
        if (tIsLeftHeld)
        {
            mDrag.follow(tMouseX, tMouseY);
        }
        else
        {
            finishItemDrag(tMouseX, tMouseY);
        }
    }

    mTooltip.update(tMouseX, tMouseY);

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

    Rml::Vector2f tClamped = mRmlUiLayer.clampDocumentToScreen(mDocument.get(), tWindow, Rml::Vector2f(tWantedLeft, tWantedTop));
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
