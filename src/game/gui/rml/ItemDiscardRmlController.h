#ifndef LAKOT_ITEM_DISCARD_RML_CONTROLLER_H
#define LAKOT_ITEM_DISCARD_RML_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

namespace lakot
{

class Localization;
class RmlUiLayer;

class ItemDiscardRmlController
{
public:
    using ConfirmCallback = std::function<void(uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, bool pIsDestroy)>;

    explicit ItemDiscardRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization);
    ~ItemDiscardRmlController();

    ItemDiscardRmlController(const ItemDiscardRmlController&) = delete;
    ItemDiscardRmlController& operator=(const ItemDiscardRmlController&) = delete;

    void setConfirmCallback(ConfirmCallback pCallback);

    void open(uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, const std::string& pName, const std::string& pIcon);
    void close();
    bool isOpen() const;

private:
    RmlUiLayer& mRmlUiLayer;
    const Localization& mLocalization;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    bool mIsOpen{false};

    uint32_t mBagSlot{0};
    uint32_t mTemplateId{0};

    Rml::String mIcon;
    Rml::String mItemName;
    bool mIsStack{false};
    int mMaxCount{0};
    Rml::String mAmountLabel;
    Rml::String mCountText;
    Rml::String mStatusMessage;
    bool mIsError{false};

    ConfirmCallback mConfirmCallback;

    void onDropClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onDestroyClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCancelClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCountKeyDown(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void confirm(bool pIsDestroy);
    void setError(const std::string& pMessage);
    void dirtyAll();
};

}

#endif
