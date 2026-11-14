#include "ItemDiscardRmlController.h"

#include <algorithm>
#include <cctype>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Input.h>

#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

namespace
{
    constexpr size_t kMaxCountDigits = 9;

    std::string trim(const std::string& pText)
    {
        const char* tSpaces = " \t\r\n";

        size_t tFirst = pText.find_first_not_of(tSpaces);

        if (tFirst == std::string::npos)
        {
            return std::string();
        }

        size_t tLast = pText.find_last_not_of(tSpaces);

        return pText.substr(tFirst, tLast - tFirst + 1);
    }
}

ItemDiscardRmlController::ItemDiscardRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("item_discard");

    if (!tConstructor)
    {
        return;
    }

    tConstructor.Bind("icon", &mIcon);
    tConstructor.Bind("item_name", &mItemName);
    tConstructor.Bind("is_stack", &mIsStack);
    tConstructor.Bind("max_count", &mMaxCount);
    tConstructor.Bind("amount_label", &mAmountLabel);
    tConstructor.Bind("count_text", &mCountText);
    tConstructor.Bind("status_message", &mStatusMessage);
    tConstructor.Bind("is_error", &mIsError);

    tConstructor.BindEventCallback("drop", &ItemDiscardRmlController::onDropClicked, this);
    tConstructor.BindEventCallback("destroy", &ItemDiscardRmlController::onDestroyClicked, this);
    tConstructor.BindEventCallback("cancel", &ItemDiscardRmlController::onCancelClicked, this);
    tConstructor.BindEventCallback("count_keydown", &ItemDiscardRmlController::onCountKeyDown, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/itemdiscard.rml");
}

ItemDiscardRmlController::~ItemDiscardRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("item_discard");
    }
}

void ItemDiscardRmlController::setConfirmCallback(ConfirmCallback pCallback)
{
    mConfirmCallback = std::move(pCallback);
}

void ItemDiscardRmlController::open(uint32_t pBagSlot, uint32_t pTemplateId, uint32_t pCount, const std::string& pName, const std::string& pIcon)
{
    mBagSlot = pBagSlot;
    mTemplateId = pTemplateId;

    mIcon = pIcon;
    mItemName = pName;
    mIsStack = pCount > 1;
    mMaxCount = static_cast<int>(pCount);
    mAmountLabel = mLocalization.format("itemdiscard.amount", { { "max", std::to_string(pCount) } });
    mCountText = mIsStack ? std::to_string(pCount) : std::string("1");
    mStatusMessage.clear();
    mIsError = false;

    dirtyAll();

    mIsOpen = true;

    if (mDocument)
    {
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void ItemDiscardRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool ItemDiscardRmlController::isOpen() const
{
    return mIsOpen;
}

void ItemDiscardRmlController::onDropClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    confirm(false);
}

void ItemDiscardRmlController::onDestroyClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    confirm(true);
}

void ItemDiscardRmlController::onCancelClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void ItemDiscardRmlController::onCountKeyDown(Rml::DataModelHandle /*pHandle*/, Rml::Event& pEvent, const Rml::VariantList& /*pArguments*/)
{
    auto tKeyIdentifier = static_cast<Rml::Input::KeyIdentifier>(pEvent.GetParameter<int>("key_identifier", 0));

    if (tKeyIdentifier == Rml::Input::KI_RETURN || tKeyIdentifier == Rml::Input::KI_NUMPADENTER)
    {
        pEvent.StopPropagation();
        confirm(false);
    }
    else if (tKeyIdentifier == Rml::Input::KI_ESCAPE)
    {
        pEvent.StopPropagation();
        close();
    }
}

void ItemDiscardRmlController::confirm(bool pIsDestroy)
{
    if (!mIsOpen)
    {
        return;
    }

    uint32_t tCount = 0;

    if (mIsStack)
    {
        std::string tText = trim(mCountText);

        bool tIsNumber = !tText.empty()
                         && tText.size() <= kMaxCountDigits
                         && std::all_of(tText.begin(), tText.end(), [](unsigned char pChar) { return std::isdigit(pChar) != 0; });

        uint32_t tParsed = tIsNumber ? static_cast<uint32_t>(std::stoul(tText)) : 0;

        if (tParsed < 1 || tParsed > static_cast<uint32_t>(mMaxCount))
        {
            setError(mLocalization.format("itemdiscard.invalid_amount", { { "max", std::to_string(mMaxCount) } }));
            return;
        }

        tCount = tParsed;
    }

    uint32_t tBagSlot = mBagSlot;
    uint32_t tTemplateId = mTemplateId;

    close();

    if (mConfirmCallback)
    {
        mConfirmCallback(tBagSlot, tTemplateId, tCount, pIsDestroy);
    }
}

void ItemDiscardRmlController::setError(const std::string& pMessage)
{
    mStatusMessage = pMessage;
    mIsError = true;

    mModelHandle.DirtyVariable("status_message");
    mModelHandle.DirtyVariable("is_error");
}

void ItemDiscardRmlController::dirtyAll()
{
    mModelHandle.DirtyVariable("icon");
    mModelHandle.DirtyVariable("item_name");
    mModelHandle.DirtyVariable("is_stack");
    mModelHandle.DirtyVariable("max_count");
    mModelHandle.DirtyVariable("amount_label");
    mModelHandle.DirtyVariable("count_text");
    mModelHandle.DirtyVariable("status_message");
    mModelHandle.DirtyVariable("is_error");
}
