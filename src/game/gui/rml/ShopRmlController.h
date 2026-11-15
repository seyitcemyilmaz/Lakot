#ifndef LAKOT_SHOP_RML_CONTROLLER_H
#define LAKOT_SHOP_RML_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "RmlDocument.h"
#include "../../network/GameTypes.h"

namespace lakot
{

class RmlUiLayer;
class InventoryController;
class Localization;
class ItemIconRenderer;

class ShopRmlController
{
public:
    using BuyCallback = std::function<void(uint64_t pNpcId, uint32_t pTemplateId, uint32_t pQuantity)>;
    using SellCallback = std::function<void(uint64_t pNpcId, uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount)>;

    ShopRmlController(RmlUiLayer& pRmlUiLayer, const InventoryController& pInventoryController, const Localization& pLocalization, ItemIconRenderer& pItemIcons);
    ~ShopRmlController();

    ShopRmlController(const ShopRmlController&) = delete;
    ShopRmlController& operator=(const ShopRmlController&) = delete;

    void setBuyCallback(BuyCallback pCallback);
    void setSellCallback(SellCallback pCallback);

    void open(uint64_t pNpcId, const std::string& pTitle, const std::vector<uint32_t>& pTemplateIds);
    void close();
    bool isOpen() const;
    uint64_t getNpcId() const;

    void setContents(const std::vector<OwnedItem>& pItems, const CharacterStats& pStats);

    void update();

private:
    struct BuyRow
    {
        int templateId = 0;
        Rml::String name;
        Rml::String icon;
        Rml::String iconStyle;
        Rml::String priceText;
        Rml::String levelText;
        bool isAffordable = true;
        bool isUsable = true;
        bool isStackable = false;
        int maxQuantity = 1;
        bool isSelected = false;
    };

    struct SellRow
    {
        int bagSlot = 0;
        int templateId = 0;
        Rml::String name;
        Rml::String icon;
        Rml::String iconStyle;
        int count = 1;
        Rml::String priceText;
        bool isPending = false;
    };

    RmlUiLayer& mRmlUiLayer;
    const InventoryController& mInventoryController;
    const Localization& mLocalization;
    ItemIconRenderer& mItemIcons;

    RmlDocument mDocument;
    Rml::DataModelHandle mModelHandle;

    bool mIsOpen{false};
    uint64_t mNpcId{0};
    std::vector<uint32_t> mTemplateIds;

    std::vector<OwnedItem> mItems;
    CharacterStats mStats;

    Rml::String mTitle;
    Rml::String mGoldText;
    Rml::String mTab{"buy"};
    int mSelectedTemplateId{0};
    int mBuyQuantity{1};
    Rml::String mBuyTotalText;
    bool mCanConfirmBuy{false};
    int mPendingSellSlot{-1};
    int mPendingSellTemplateId{0};
    std::vector<BuyRow> mBuyItems;
    std::vector<SellRow> mSellItems;

    bool mIsDragging{false};
    bool mHasBeenMoved{false};
    float mDragStartMouseX{0.0f};
    float mDragStartMouseY{0.0f};
    float mDragStartDocLeft{0.0f};
    float mDragStartDocTop{0.0f};

    BuyCallback mBuyCallback;
    SellCallback mSellCallback;

    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDragStart(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSetTab(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSelectBuy(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onQuantityUp(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onQuantityDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onConfirmBuy(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onSell(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onConfirmSell(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCancelSell(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void rebuild();
    void keepWindowOnScreen(float pMouseX, float pMouseY);
    int getSelectedMaxQuantity() const;
};

}

#endif
