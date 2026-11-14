#include "SkillWindowRmlController.h"

#include <SDL3/SDL_log.h>

#include <cmath>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>

#include "RmlUiLayer.h"
#include "../../localization/Localization.h"

using namespace lakot;

SkillWindowRmlController::SkillWindowRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const SkillCatalog& pSkillCatalog, const ClassCatalog& pClassCatalog)
    : mRmlUiLayer(pRmlUiLayer)
    , mLocalization(pLocalization)
    , mSkillCatalog(pSkillCatalog)
    , mClassCatalog(pClassCatalog)
{
    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (!tContext)
    {
        return;
    }

    Rml::DataModelConstructor tConstructor = tContext->CreateDataModel("skills");

    if (!tConstructor)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "RmlUi data model 'skills' could not be created");
        return;
    }

    static bool sTypesRegistered = false;

    if (!sTypesRegistered)
    {
        auto tPath = tConstructor.RegisterStruct<PathRow>();
        tPath.RegisterMember("id", &PathRow::id);
        tPath.RegisterMember("name", &PathRow::name);
        tPath.RegisterMember("skill_names", &PathRow::skillNames);
        tPath.RegisterMember("warning_text", &PathRow::warningText);
        tPath.RegisterMember("is_pending", &PathRow::isPending);
        tConstructor.RegisterArray<std::vector<PathRow>>();

        auto tSkill = tConstructor.RegisterStruct<SkillRow>();
        tSkill.RegisterMember("id", &SkillRow::id);
        tSkill.RegisterMember("name", &SkillRow::name);
        tSkill.RegisterMember("description", &SkillRow::description);
        tSkill.RegisterMember("icon", &SkillRow::icon);
        tSkill.RegisterMember("level_text", &SkillRow::levelText);
        tSkill.RegisterMember("is_learned", &SkillRow::isLearned);
        tSkill.RegisterMember("is_locked", &SkillRow::isLocked);
        tSkill.RegisterMember("lock_text", &SkillRow::lockText);
        tSkill.RegisterMember("can_raise", &SkillRow::canRaise);
        tSkill.RegisterMember("stats_text", &SkillRow::statsText);
        tSkill.RegisterMember("tier_text", &SkillRow::tierText);
        tConstructor.RegisterArray<std::vector<SkillRow>>();

        sTypesRegistered = true;
    }

    tConstructor.Bind("class_name", &mClassName);
    tConstructor.Bind("path_name", &mPathName);
    tConstructor.Bind("has_path", &mHasPath);
    tConstructor.Bind("can_choose_path", &mCanChoosePath);
    tConstructor.Bind("skill_points", &mSkillPoints);
    tConstructor.Bind("has_skill_points", &mHasSkillPoints);
    tConstructor.Bind("pending_path_id", &mPendingPathId);
    tConstructor.Bind("paths", &mPaths);
    tConstructor.Bind("skills", &mSkills);

    tConstructor.BindEventCallback("close", &SkillWindowRmlController::onCloseClicked, this);
    tConstructor.BindEventCallback("raise_skill", &SkillWindowRmlController::onRaiseSkillClicked, this);
    tConstructor.BindEventCallback("choose_path", &SkillWindowRmlController::onChoosePathClicked, this);
    tConstructor.BindEventCallback("confirm_path", &SkillWindowRmlController::onConfirmPathClicked, this);
    tConstructor.BindEventCallback("cancel_path", &SkillWindowRmlController::onCancelPathClicked, this);

    mModelHandle = tConstructor.GetModelHandle();

    mDocument.load(mRmlUiLayer, "ui/skills.rml");
}

SkillWindowRmlController::~SkillWindowRmlController()
{
    mDocument.reset();

    Rml::Context* tContext = mRmlUiLayer.getContext();

    if (tContext)
    {
        tContext->RemoveDataModel("skills");
    }
}

void SkillWindowRmlController::setStats(const CharacterStats& pStats)
{
    mStats = pStats;

    if (mStats.pathId != 0)
    {
        mPendingPathId = 0;
    }

    rebuild();
}

void SkillWindowRmlController::setRaiseSkillCallback(RaiseSkillCallback pCallback)
{
    mRaiseSkillCallback = std::move(pCallback);
}

void SkillWindowRmlController::setChoosePathCallback(ChoosePathCallback pCallback)
{
    mChoosePathCallback = std::move(pCallback);
}

void SkillWindowRmlController::rebuild()
{
    const ClassDefinition* tClass = mClassCatalog.find(mStats.classId);
    const ClassPath* tPath = tClass ? tClass->findPath(mStats.pathId) : nullptr;

    mClassName = tClass ? mLocalization.content("class." + tClass->key + ".name", tClass->name) : Rml::String();
    mPathName = tPath ? mLocalization.content("path." + tClass->key + "." + tPath->key + ".name", tPath->name) : Rml::String();
    mHasPath = tPath != nullptr;
    mCanChoosePath = tClass && !mHasPath && mStats.level >= mClassCatalog.getPathUnlockLevel();
    mSkillPoints = 0;
    mHasSkillPoints = false;

    mPaths.clear();
    mSkills.clear();

    if (tClass)
    {
        std::vector<const SkillDefinition*> tClassSkills = mSkillCatalog.getForClass(tClass->id);

        for (const SkillDefinition* tSkill : tClassSkills)
        {
            if (tSkill->isFixed() || (tPath && tSkill->pathId == tPath->id))
            {
                mSkills.push_back(makeSkillRow(*tSkill));
            }
        }

        if (!mHasPath)
        {
            for (const ClassPath& tCandidate : tClass->paths)
            {
                PathRow tRow;
                tRow.id = static_cast<int>(tCandidate.id);
                tRow.name = mLocalization.content("path." + tClass->key + "." + tCandidate.key + ".name", tCandidate.name);
                tRow.warningText = mLocalization.format("skills.path_warning", { { "name", tRow.name } });
                tRow.isPending = mPendingPathId == tRow.id;

                for (const SkillDefinition* tSkill : tClassSkills)
                {
                    if (tSkill->pathId == tCandidate.id)
                    {
                        tRow.skillNames += tRow.skillNames.empty() ? tSkill->name : " \xC2\xB7 " + tSkill->name;
                    }
                }

                mPaths.push_back(std::move(tRow));
            }
        }
    }

    mModelHandle.DirtyAllVariables();
}

SkillWindowRmlController::SkillRow SkillWindowRmlController::makeSkillRow(const SkillDefinition& pSkill) const
{
    uint32_t tMaxLevel = mSkillCatalog.getMaxSkillLevel();
    uint32_t tLevel = pSkill.isFixed() ? 1 : 0;
    bool tIsLocked = mStats.level < pSkill.unlockLevel;

    SkillRow tRow;
    tRow.id = static_cast<int>(pSkill.id);
    tRow.name = mLocalization.content("skill." + pSkill.key + ".name", pSkill.name);
    tRow.description = mLocalization.content("skill." + pSkill.key + ".desc", pSkill.description);
    tRow.icon = "textures/skills/" + pSkill.key + ".tga";
    tRow.levelText = std::to_string(tLevel) + "/" + std::to_string(tMaxLevel);
    tRow.isLearned = tLevel > 0;
    tRow.isLocked = tIsLocked;
    tRow.lockText = tIsLocked ? mLocalization.format("skills.lock_level", { { "level", std::to_string(pSkill.unlockLevel) } }) : Rml::String();
    tRow.canRaise = !tIsLocked && tLevel < tMaxLevel;
    tRow.statsText = mLocalization.format("skills.stats", { { "mp", std::to_string(pSkill.mpCost) }, { "cooldown", formatCooldown(pSkill.cooldown) } });

    return tRow;
}

Rml::String SkillWindowRmlController::formatCooldown(float pSeconds) const
{
    if (pSeconds <= 0.0f)
    {
        return mLocalization.text("skills.cooldown_none");
    }

    if (std::fabs(pSeconds - std::round(pSeconds)) < 0.05f)
    {
        return mLocalization.format("skills.cooldown_seconds", { { "seconds", mLocalization.formatDecimal(std::round(pSeconds), 0) } });
    }

    return mLocalization.format("skills.cooldown_seconds", { { "seconds", mLocalization.formatDecimal(pSeconds, 1) } });
}

void SkillWindowRmlController::onCloseClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    close();
}

void SkillWindowRmlController::onRaiseSkillClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || !mRaiseSkillCallback)
    {
        return;
    }

    int tSkillId = pArguments[0].Get<int>(0);

    if (tSkillId > 0)
    {
        mRaiseSkillCallback(static_cast<uint32_t>(tSkillId));
    }
}

void SkillWindowRmlController::onChoosePathClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& pArguments)
{
    if (pArguments.empty() || !mCanChoosePath)
    {
        return;
    }

    mPendingPathId = pArguments[0].Get<int>(0);
    rebuild();
}

void SkillWindowRmlController::onConfirmPathClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    int tPathId = mPendingPathId;

    mPendingPathId = 0;
    rebuild();

    if (tPathId > 0 && mCanChoosePath && mChoosePathCallback)
    {
        mChoosePathCallback(static_cast<uint32_t>(tPathId));
    }
}

void SkillWindowRmlController::onCancelPathClicked(Rml::DataModelHandle /*pHandle*/, Rml::Event& /*pEvent*/, const Rml::VariantList& /*pArguments*/)
{
    mPendingPathId = 0;
    rebuild();
}

void SkillWindowRmlController::toggle()
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
    rebuild();

    mDocument->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    mDocument->PullToFront();
}

void SkillWindowRmlController::close()
{
    if (!mIsOpen)
    {
        return;
    }

    mIsOpen = false;
    mPendingPathId = 0;

    if (mDocument)
    {
        mDocument->Hide();
    }
}

bool SkillWindowRmlController::isOpen() const
{
    return mIsOpen;
}
