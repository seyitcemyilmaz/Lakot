#include "WorldOverlay.h"

#include <algorithm>
#include <cmath>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ID.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/StringUtilities.h>

#include "../../gui/rml/RmlUiLayer.h"

using namespace lakot;

namespace
{
    const char* getNameToneClass(WorldOverlay::NameToneType pTone)
    {
        switch (pTone)
        {
            case WorldOverlay::NameToneType::eHostilePlayer: return "wo-name tone-hostile";
            case WorldOverlay::NameToneType::eNpc:           return "wo-name tone-npc";
            case WorldOverlay::NameToneType::eMonster:       return "wo-name tone-monster";
            case WorldOverlay::NameToneType::eGroundItem:    return "wo-name tone-item";
            case WorldOverlay::NameToneType::eDead:          return "wo-name tone-dead";
            case WorldOverlay::NameToneType::ePlayer:        break;
        }

        return "wo-name";
    }

    const char* getNumberToneClass(WorldOverlay::NumberToneType pTone)
    {
        switch (pTone)
        {
            case WorldOverlay::NumberToneType::eAgainstLocal: return "wo-number num-against";
            case WorldOverlay::NumberToneType::eByLocal:      return "wo-number num-local";
            case WorldOverlay::NumberToneType::eOther:        break;
        }

        return "wo-number";
    }

    Rml::Element* appendDiv(Rml::Element* pParent, const char* pClass)
    {
        Rml::ElementPtr tElement = pParent->GetOwnerDocument()->CreateElement("div");
        tElement->SetClassNames(pClass);
        return pParent->AppendChild(std::move(tElement));
    }

    void setPosition(Rml::Element* pElement, float pX, float pY)
    {
        pElement->SetProperty(Rml::PropertyId::Left, Rml::Property(std::round(pX), Rml::Unit::PX));
        pElement->SetProperty(Rml::PropertyId::Top, Rml::Property(std::round(pY), Rml::Unit::PX));
    }

    void setText(Rml::Element* pElement, const std::string& pText)
    {
        pElement->SetInnerRML(Rml::StringUtilities::EncodeRml(pText));
    }
}

WorldOverlay::WorldOverlay(RmlUiLayer& pRmlUiLayer)
    : mRmlUiLayer(pRmlUiLayer)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("worldoverlay");
    tConstructor.Bind("menu_open", &mIsMenuOpen);
    tConstructor.Bind("menu_name", &mMenuName);
    tConstructor.Bind("menu_left", &mMenuLeft);
    tConstructor.Bind("menu_top", &mMenuTop);
    tConstructor.BindEventCallback("menu_chat", &WorldOverlay::onMenuChatClicked, this);
    mModel = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/worldoverlay.rml");

    if (mDocument)
    {
        mPlateParent = mDocument->GetElementById("plates");
        mNumberParent = mDocument->GetElementById("numbers");
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    }
}

WorldOverlay::~WorldOverlay()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("worldoverlay");
    }
}

void WorldOverlay::setMenuChatCallback(MenuChatCallback pCallback)
{
    mMenuChatCallback = std::move(pCallback);
}

void WorldOverlay::beginFrame()
{
    mPlatesUsed = 0;
    mNumbersUsed = 0;
}

WorldOverlay::Plate& WorldOverlay::acquirePlate()
{
    if (mPlatesUsed == mPlates.size())
    {
        Plate tPlate;
        tPlate.root = appendDiv(mPlateParent, "wo-plate");
        tPlate.name = appendDiv(tPlate.root, "wo-name");
        tPlate.barFill = appendDiv(appendDiv(tPlate.root, "wo-bar"), "wo-bar-fill");
        tPlate.speech = appendDiv(tPlate.root, "wo-speech");
        mPlates.push_back(std::move(tPlate));
    }

    return mPlates[mPlatesUsed++];
}

WorldOverlay::Number& WorldOverlay::acquireNumber()
{
    if (mNumbersUsed == mNumbers.size())
    {
        Number tNumber;
        tNumber.root = appendDiv(mNumberParent, "wo-number");
        mNumbers.push_back(std::move(tNumber));
    }

    return mNumbers[mNumbersUsed++];
}

void WorldOverlay::showNameplate(float pX, float pY, const std::string& pText, NameToneType pTone, int pHealthPercent, const std::string& pSpeech)
{
    if (!mPlateParent)
    {
        return;
    }

    Plate& tPlate = acquirePlate();

    if (!tPlate.isVisible)
    {
        tPlate.root->SetClass("wo-hidden", false);
        tPlate.isVisible = true;
    }

    setPosition(tPlate.root, pX, pY);

    if (tPlate.text != pText)
    {
        tPlate.text = pText;
        setText(tPlate.name, pText);
    }

    if (!tPlate.hasToneApplied || tPlate.tone != pTone)
    {
        tPlate.tone = pTone;
        tPlate.hasToneApplied = true;
        tPlate.name->SetClassNames(getNameToneClass(pTone));
    }

    int tHealth = pHealthPercent < 0 ? -1 : std::clamp(pHealthPercent, 0, 100);

    if (tPlate.health != tHealth)
    {
        if ((tPlate.health < 0) != (tHealth < 0))
        {
            tPlate.root->SetClass("has-bar", tHealth >= 0);
        }

        tPlate.health = tHealth;

        if (tHealth >= 0)
        {
            tPlate.barFill->SetProperty(Rml::PropertyId::Width, Rml::Property(static_cast<float>(tHealth), Rml::Unit::PERCENT));
        }
    }

    if (tPlate.speechText != pSpeech)
    {
        bool tHadSpeech = !tPlate.speechText.empty();
        tPlate.speechText = pSpeech;

        if (tHadSpeech != !pSpeech.empty())
        {
            tPlate.root->SetClass("has-speech", !pSpeech.empty());
        }

        setText(tPlate.speech, pSpeech);
    }
}

void WorldOverlay::showDamageNumber(float pX, float pY, const std::string& pText, NumberToneType pTone, float pAlpha)
{
    if (!mNumberParent)
    {
        return;
    }

    Number& tNumber = acquireNumber();

    if (!tNumber.isVisible)
    {
        tNumber.root->SetClass("wo-hidden", false);
        tNumber.isVisible = true;
    }

    setPosition(tNumber.root, pX, pY);
    tNumber.root->SetProperty(Rml::PropertyId::Opacity, Rml::Property(std::clamp(pAlpha, 0.0f, 1.0f), Rml::Unit::NUMBER));

    if (tNumber.text != pText)
    {
        tNumber.text = pText;
        setText(tNumber.root, pText);
    }

    if (!tNumber.hasToneApplied || tNumber.tone != pTone)
    {
        tNumber.tone = pTone;
        tNumber.hasToneApplied = true;
        tNumber.root->SetClassNames(getNumberToneClass(pTone));
    }
}

void WorldOverlay::endFrame()
{
    for (size_t tIndex = mPlatesUsed; tIndex < mPlates.size(); ++tIndex)
    {
        if (mPlates[tIndex].isVisible)
        {
            mPlates[tIndex].root->SetClass("wo-hidden", true);
            mPlates[tIndex].isVisible = false;
        }
    }

    for (size_t tIndex = mNumbersUsed; tIndex < mNumbers.size(); ++tIndex)
    {
        if (mNumbers[tIndex].isVisible)
        {
            mNumbers[tIndex].root->SetClass("wo-hidden", true);
            mNumbers[tIndex].isVisible = false;
        }
    }
}

void WorldOverlay::openMenu(float pX, float pY, uint64_t pPlayerId, const std::string& pName)
{
    mMenuPlayerId = pPlayerId;
    mMenuName = pName;
    mMenuLeft = std::to_string(static_cast<int>(pX)) + "px";
    mMenuTop = std::to_string(static_cast<int>(pY)) + "px";
    mIsMenuOpen = true;

    mModel.DirtyVariable("menu_name");
    mModel.DirtyVariable("menu_left");
    mModel.DirtyVariable("menu_top");
    mModel.DirtyVariable("menu_open");
}

void WorldOverlay::closeMenu()
{
    if (mIsMenuOpen)
    {
        mIsMenuOpen = false;
        mModel.DirtyVariable("menu_open");
    }
}

bool WorldOverlay::isMenuOpen() const
{
    return mIsMenuOpen;
}

void WorldOverlay::onMenuChatClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    uint64_t tPlayerId = mMenuPlayerId;
    std::string tName = mMenuName;

    closeMenu();

    if (mMenuChatCallback)
    {
        mMenuChatCallback(tPlayerId, tName);
    }
}
