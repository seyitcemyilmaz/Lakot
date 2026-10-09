#include "CharacterSheetRmlController.h"

#include <algorithm>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#include "RmlUiLayer.h"

using namespace lakot;

Rml::String CharacterSheetRmlController::toPercent(int64_t pValue, int64_t pMaximum)
{
    if (pMaximum <= 0)
    {
        return Rml::String("0%");
    }

    int64_t tPercent = std::clamp<int64_t>((pValue * 100) / pMaximum, 0, 100);

    return Rml::String(std::to_string(tPercent) + "%");
}

CharacterSheetRmlController::CharacterSheetRmlController(RmlUiLayer& pRmlUiLayer, const std::string& pCharacterName)
    : mRmlUiLayer(pRmlUiLayer)
    , mName(pCharacterName)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("character");

    tConstructor.Bind("name", &mName);
    tConstructor.Bind("level", &mLevel);

    tConstructor.Bind("health", &mHealth);
    tConstructor.Bind("mana", &mMana);
    tConstructor.Bind("experience", &mExperience);

    tConstructor.Bind("health_percent", &mHealthPercent);
    tConstructor.Bind("mana_percent", &mManaPercent);
    tConstructor.Bind("experience_percent", &mExperiencePercent);

    tConstructor.Bind("strength", &mStrength);
    tConstructor.Bind("dexterity", &mDexterity);
    tConstructor.Bind("intelligence", &mIntelligence);
    tConstructor.Bind("vitality", &mVitality);

    tConstructor.Bind("attack", &mAttack);
    tConstructor.Bind("defense", &mDefense);
    tConstructor.Bind("gold", &mGold);

    tConstructor.BindEventCallback("close", &CharacterSheetRmlController::onCloseClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument = mRmlUiLayer.loadDocument("ui/character.rml");

    // Loaded but not shown - C toggles it.
}

CharacterSheetRmlController::~CharacterSheetRmlController()
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        if (mDocument)
        {
            tContext->UnloadDocument(mDocument);
        }

        tContext->RemoveDataModel("character");
    }
}

void CharacterSheetRmlController::setStats(const CharacterStats& pStats)
{
    mLevel = std::to_string(pStats.level);

    mHealth = std::to_string(pStats.health) + " / " + std::to_string(pStats.maxHealth);
    mMana = std::to_string(pStats.mana) + " / " + std::to_string(pStats.maxMana);
    mExperience = std::to_string(pStats.experience) + " / " + std::to_string(pStats.experienceForNextLevel);

    mHealthPercent = toPercent(pStats.health, pStats.maxHealth);
    mManaPercent = toPercent(pStats.mana, pStats.maxMana);
    mExperiencePercent = toPercent(static_cast<int64_t>(pStats.experience),
                                   static_cast<int64_t>(pStats.experienceForNextLevel));

    mStrength = std::to_string(pStats.strength);
    mDexterity = std::to_string(pStats.dexterity);
    mIntelligence = std::to_string(pStats.intelligence);
    mVitality = std::to_string(pStats.vitality);

    mAttack = std::to_string(pStats.attack);
    mDefense = std::to_string(pStats.defense);
    mGold = std::to_string(pStats.gold);

    dirtyAll();
}

void CharacterSheetRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void CharacterSheetRmlController::toggle()
{
    if (mIsOpen)
    {
        close();
        return;
    }

    mIsOpen = true;
    dirtyAll();

    if (mDocument)
    {
        // FocusFlag::None, same as the bag - opening a panel must not take
        // keyboard focus away from movement.
        mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
        mDocument->PullToFront();
    }
}

void CharacterSheetRmlController::close()
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

bool CharacterSheetRmlController::isOpen() const
{
    return mIsOpen;
}

void CharacterSheetRmlController::dirtyAll()
{
    mModelHandle.DirtyAllVariables();
}
