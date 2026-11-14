#include "ShopRmlController.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataStructHandle.h>

#include "RmlUiLayer.h"

#include "../../item/ItemIconPath.h"
#include "../../item/ItemIconRenderer.h"
#include "../../localization/Localization.h"
#include "../../network/controller/InventoryController.h"

using namespace lakot;

namespace
{
    Rml::String toPixels(int pValue)
    {
        return Rml::String(std::to_string(pValue) + "px");
    }

    int getIntArgument(const Rml::VariantList& pArguments, int pFallback)
    {
        return pArguments.empty() ? pFallback : pArguments[0].Get<int>(pFallback);
    }
}

ShopRmlController::ShopRmlController(RmlUiLayer& pRmlUiLayer, const InventoryController& pInventoryController, const Localization& pLocalization, ItemIconRenderer& pItemIcons)
    : mRmlUiLayer(pRmlUiLayer)
    , mInventoryController(pInventoryController)
    , mLocalization(pLocalization)
    , mItemIcons(pItemIcons)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("shop");

    if (!tConstructor)
    {
        return;
    }

    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tBuy = tConstructor.RegisterStruct<BuyRow>();
        tBuy.RegisterMember("template_id", &BuyRow::templateId);
        tBuy.RegisterMember("name", &BuyRow::name);
        tBuy.RegisterMember("icon", &BuyRow::icon);
        tBuy.RegisterMember("price_text", &BuyRow::priceText);
        tBuy.RegisterMember("level_text", &BuyRow::levelText);
        tBuy.RegisterMember("is_affordable", &BuyRow::isAffordable);
        tBuy.RegisterMember("is_usable", &BuyRow::isUsable);
        tBuy.RegisterMember("is_stackable", &BuyRow::isStackable);
        tBuy.RegisterMember("max_quantity", &BuyRow::maxQuantity);
        tBuy.RegisterMember("is_selected", &BuyRow::isSelected);
        tConstructor.RegisterArray<std::vector<BuyRow>>();

        auto tSell = tConstructor.RegisterStruct<SellRow>();
        tSell.RegisterMember("bag_slot", &SellRow::bagSlot);
        tSell.RegisterMember("template_id", &SellRow::templateId);
        tSell.RegisterMember("name", &SellRow::name);
        tSell.RegisterMember("icon", &SellRow::icon);
        tSell.RegisterMember("count", &SellRow::count);
        tSell.RegisterMember("price_text", &SellRow::priceText);
        tSell.RegisterMember("is_pending", &SellRow::isPending);
        tConstructor.RegisterArray<std::vector<SellRow>>();

        sTypesRegistered = true;
    }

    tConstructor.Bind("title", &mTitle);
    tConstructor.Bind("gold_text", &mGoldText);
    tConstructor.Bind("tab", &mTab);
    tConstructor.Bind("selected_template_id", &mSelectedTemplateId);
    tConstructor.Bind("buy_quantity", &mBuyQuantity);
    tConstructor.Bind("buy_total_text", &mBuyTotalText);
    tConstructor.Bind("can_confirm_buy", &mCanConfirmBuy);
    tConstructor.Bind("pending_sell_slot", &mPendingSellSlot);
    tConstructor.Bind("buy_items", &mBuyItems);
    tConstructor.Bind("sell_items", &mSellItems);

    tConstructor.BindEventCallback("close", &ShopRmlController::onCloseClicked, this);
    tConstructor.BindEventCallback("drag_start", &ShopRmlController::onDragStart, this);
    tConstructor.BindEventCallback("set_tab", &ShopRmlController::onSetTab, this);
    tConstructor.BindEventCallback("select_buy", &ShopRmlController::onSelectBuy, this);
    tConstructor.BindEventCallback("quantity_up", &ShopRmlController::onQuantityUp, this);
    tConstructor.BindEventCallback("quantity_down", &ShopRmlController::onQuantityDown, this);
    tConstructor.BindEventCallback("confirm_buy", &ShopRmlController::onConfirmBuy, this);
    tConstructor.BindEventCallback("sell", &ShopRmlController::onSell, this);
    tConstructor.BindEventCallback("confirm_sell", &ShopRmlController::onConfirmSell, this);
    tConstructor.BindEventCallback("cancel_sell", &ShopRmlController::onCancelSell, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/shop.rml");
}

ShopRmlController::~ShopRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("shop");
    }
}

void ShopRmlController::setBuyCallback(BuyCallback pCallback)
{
    mBuyCallback = std::move(pCallback);
}

void ShopRmlController::setSellCallback(SellCallback pCallback)
{
    mSellCallback = std::move(pCallback);
}

void ShopRmlController::open(uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds)
{
    mNpcId = pNpcId;
    mTemplateIds = pTemplateIds;
    mTitle = pTitle;
    mTab = "buy";
    mSelectedTemplateId = 0;
    mBuyQuantity = 1;
    mPendingSellSlot = -1;
    mPendingSellTemplateId = 0;

    rebuild();

    if (mIsOpen)
    {
        return;
    }

    mIsOpen = true;

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void ShopRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;
    mIsDragging = false;
    mNpcId = 0;

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool ShopRmlController::isOpen() const
{
    return mIsOpen;
}

uint64_t ShopRmlController::getNpcId() const
{
    return mNpcId;
}

void ShopRmlController::setContents(const std::vector<OwnedItem>& pItems, const CharacterStats& pStats)
{
    mItems = pItems;
    mStats = pStats;

    if (mIsOpen)
    {
        rebuild();
    }
}

void ShopRmlController::update()
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
}

int ShopRmlController::getSelectedMaxQuantity() const
{
    const ItemTemplate* tTemplate = mInventoryController.findTemplate(static_cast<uint32_t>(mSelectedTemplateId));

    if (!tTemplate || tTemplate->maxStack < 1)
    {
        return 1;
    }

    return static_cast<int>(tTemplate->maxStack);
}

void ShopRmlController::rebuild()
{
    mGoldText = std::to_string(mStats.gold);

    mBuyItems.clear();

    bool tHasSelection = false;

    for (uint32_t tTemplateId : mTemplateIds)
    {
        const ItemTemplate* tTemplate = mInventoryController.findTemplate(tTemplateId);

        if (!tTemplate)
        {
            continue;
        }

        BuyRow tRow;
        tRow.templateId = static_cast<int>(tTemplateId);
        tRow.name = mLocalization.content("item." + std::to_string(tTemplate->templateId) + ".name", tTemplate->name);
        tRow.icon = ItemIconPath::resolve(mItemIcons, tTemplate);
        tRow.priceText = std::to_string(tTemplate->price);
        tRow.levelText = tTemplate->requiredLevel > 1
            ? mLocalization.format("shop.level_requirement", { { "level", std::to_string(tTemplate->requiredLevel) } })
            : std::string();
        tRow.isAffordable = mStats.gold >= tTemplate->price;
        tRow.isUsable = tTemplate->isAllowedForClass(mStats.classId) && mStats.level >= tTemplate->requiredLevel;
        tRow.isStackable = tTemplate->maxStack > 1;
        tRow.maxQuantity = static_cast<int>(std::max<uint32_t>(tTemplate->maxStack, 1));
        tRow.isSelected = static_cast<int>(tTemplateId) == mSelectedTemplateId;
        tHasSelection = tHasSelection || tRow.isSelected;

        mBuyItems.push_back(std::move(tRow));
    }

    if (!tHasSelection)
    {
        mSelectedTemplateId = 0;
    }

    mBuyQuantity = std::clamp(mBuyQuantity, 1, mSelectedTemplateId != 0 ? getSelectedMaxQuantity() : 1);

    mCanConfirmBuy = false;
    mBuyTotalText.clear();

    if (const ItemTemplate* tSelected = mSelectedTemplateId != 0 ? mInventoryController.findTemplate(static_cast<uint32_t>(mSelectedTemplateId)) : nullptr)
    {
        uint64_t tTotal = static_cast<uint64_t>(tSelected->price) * static_cast<uint64_t>(mBuyQuantity);
        mBuyTotalText = std::to_string(tTotal);
        mCanConfirmBuy = mStats.gold >= tTotal;
    }

    mSellItems.clear();

    bool tHasPending = false;

    for (const OwnedItem& tItem : mItems)
    {
        if (tItem.location != ItemLocationType::eInventory)
        {
            continue;
        }

        const ItemTemplate* tTemplate = mInventoryController.findTemplate(tItem.templateId);

        if (!tTemplate || tTemplate->sellPrice == 0)
        {
            continue;
        }

        SellRow tRow;
        tRow.bagSlot = static_cast<int>(tItem.slot);
        tRow.templateId = static_cast<int>(tItem.templateId);
        tRow.name = mLocalization.content("item." + std::to_string(tTemplate->templateId) + ".name", tTemplate->name);
        tRow.icon = ItemIconPath::resolve(mItemIcons, tTemplate);
        tRow.count = static_cast<int>(tItem.count);
        tRow.priceText = std::to_string(static_cast<uint64_t>(tTemplate->sellPrice) * tItem.count);
        tRow.isPending = tRow.bagSlot == mPendingSellSlot && tRow.templateId == mPendingSellTemplateId;
        tHasPending = tHasPending || tRow.isPending;

        mSellItems.push_back(std::move(tRow));
    }

    if (!tHasPending)
    {
        mPendingSellSlot = -1;
    mPendingSellTemplateId = 0;
    }

    mModelHandle.DirtyAllVariables();
}

void ShopRmlController::keepWindowOnScreen(float pMouseX, float pMouseY)
{
    Rml::Element* tWindow = mDocument->QuerySelector(".shop-window");

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

    if (std::abs(tClamped.x - tDocLeft) >= 0.5f || std::abs(tClamped.y - tDocTop) >= 0.5f)
    {
        mDocument->SetProperty("left", toPixels(static_cast<int>(std::lround(tClamped.x))));
        mDocument->SetProperty("top", toPixels(static_cast<int>(std::lround(tClamped.y))));
    }
}

void ShopRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void ShopRmlController::onDragStart(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    if (pEvent.GetParameter<int>("button", -1) != 0 || !mDocument)
    {
        return;
    }

    mIsDragging = true;
    mHasBeenMoved = true;

    SDL_GetMouseState(&mDragStartMouseX, &mDragStartMouseY);
    mDragStartDocLeft = mDocument->GetAbsoluteLeft();
    mDragStartDocTop = mDocument->GetAbsoluteTop();
}

void ShopRmlController::onSetTab(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty())
    {
        return;
    }

    Rml::String tTab = pArguments[0].Get<Rml::String>();

    if (tTab != "buy" && tTab != "sell")
    {
        return;
    }

    mTab = tTab;
    mPendingSellSlot = -1;
    mPendingSellTemplateId = 0;

    rebuild();
}

void ShopRmlController::onSelectBuy(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    int tTemplateId = getIntArgument(pArguments, 0);

    if (tTemplateId == mSelectedTemplateId)
    {
        return;
    }

    mSelectedTemplateId = tTemplateId;
    mBuyQuantity = 1;

    rebuild();
}

void ShopRmlController::onQuantityUp(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mBuyQuantity += 1;

    rebuild();
}

void ShopRmlController::onQuantityDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mBuyQuantity -= 1;

    rebuild();
}

void ShopRmlController::onConfirmBuy(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (!mIsOpen || !mCanConfirmBuy || mSelectedTemplateId == 0 || !mBuyCallback)
    {
        return;
    }

    mBuyCallback(mNpcId, static_cast<uint32_t>(mSelectedTemplateId), static_cast<uint32_t>(mBuyQuantity));
}

void ShopRmlController::onSell(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    mPendingSellSlot = getIntArgument(pArguments, -1);
    mPendingSellTemplateId = 0;

    for (const SellRow& tRow : mSellItems)
    {
        if (tRow.bagSlot == mPendingSellSlot)
        {
            mPendingSellTemplateId = tRow.templateId;
        }
    }

    if (mPendingSellTemplateId == 0)
    {
        mPendingSellSlot = -1;
    }

    rebuild();
}

void ShopRmlController::onConfirmSell(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    if (!mIsOpen || mPendingSellSlot < 0 || !mSellCallback)
    {
        return;
    }

    auto tFound = std::find_if(mSellItems.begin(), mSellItems.end(), [this](const SellRow& pRow)
    {
        return pRow.bagSlot == mPendingSellSlot && pRow.templateId == mPendingSellTemplateId;
    });

    if (tFound == mSellItems.end())
    {
        mPendingSellSlot = -1;
        mPendingSellTemplateId = 0;

        rebuild();
        return;
    }

    uint32_t tBagSlot = static_cast<uint32_t>(tFound->bagSlot);
    uint32_t tTemplateId = static_cast<uint32_t>(tFound->templateId);

    mPendingSellSlot = -1;
    mPendingSellTemplateId = 0;

    rebuild();

    mSellCallback(mNpcId, tBagSlot, tTemplateId, 0);
}

void ShopRmlController::onCancelSell(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mPendingSellSlot = -1;
    mPendingSellTemplateId = 0;

    rebuild();
}
