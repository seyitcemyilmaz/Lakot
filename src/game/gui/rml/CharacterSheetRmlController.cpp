#include "CharacterSheetRmlController.h"

#include <SDL3/SDL_log.h>

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

CharacterSheetRmlController::CharacterSheetRmlController(RmlUiLayer& pRmlUiLayer, const ClassCatalog& pClassCatalog, const std::string& pCharacterName)
    : mRmlUiLayer(pRmlUiLayer)
    , mClassCatalog(pClassCatalog)
    , mName(pCharacterName)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("character");

    if (!tConstructor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "RmlUi data model 'character' could not be created");
        return;
    }

    tConstructor.Bind("name", &mName);
    tConstructor.Bind("level", &mLevel);
    tConstructor.Bind("class_name", &mClassName);
    tConstructor.Bind("path_name", &mPathName);
    tConstructor.Bind("stat_points", &mStatPoints);
    tConstructor.Bind("has_stat_points", &mHasStatPoints);
    tConstructor.Bind("can_raise_strength", &mCanRaiseStrength);
    tConstructor.Bind("can_raise_dexterity", &mCanRaiseDexterity);
    tConstructor.Bind("can_raise_intelligence", &mCanRaiseIntelligence);
    tConstructor.Bind("can_raise_vitality", &mCanRaiseVitality);

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
    tConstructor.BindEventCallback("raise_stat", &CharacterSheetRmlController::onRaiseStatClicked, this);

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

    const ClassDefinition* tClass = mClassCatalog.find(pStats.classId);
    const ClassPath* tPath = tClass ? tClass->findPath(pStats.pathId) : nullptr;

    mClassName = tClass ? tClass->name : std::string();
    mPathName = tPath ? tPath->name : std::string();
    mStatPoints = std::to_string(pStats.statPoints);

    uint32_t tCap = mClassCatalog.getStatCap();
    bool tHasPoints = pStats.statPoints > 0;

    mHasStatPoints = tHasPoints;
    mCanRaiseStrength = tHasPoints && pStats.strength < static_cast<int32_t>(tCap);
    mCanRaiseDexterity = tHasPoints && pStats.dexterity < static_cast<int32_t>(tCap);
    mCanRaiseIntelligence = tHasPoints && pStats.intelligence < static_cast<int32_t>(tCap);
    mCanRaiseVitality = tHasPoints && pStats.vitality < static_cast<int32_t>(tCap);

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

void CharacterSheetRmlController::setAllocateStatCallback(AllocateStatCallback pCallback)
{
    mAllocateStatCallback = std::move(pCallback);
}

void CharacterSheetRmlController::onRaiseStatClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || !mAllocateStatCallback)
    {
        return;
    }

    int tStat = pArguments[0].Get<int>(0);

    if (!services::progression::StatType_IsValid(tStat) || tStat == services::progression::STAT_TYPE_UNSPECIFIED)
    {
        return;
    }

    mAllocateStatCallback(static_cast<services::progression::StatType>(tStat), 1);
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

    if (!mDocument)
    {
        return;
    }

    mIsOpen = true;
    dirtyAll();

    // FocusFlag::None, same as the bag - opening a panel must not take
    // keyboard focus away from movement.
    mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    mDocument->PullToFront();
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
