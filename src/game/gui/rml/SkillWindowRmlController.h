#ifndef LAKOT_SKILL_WINDOW_RML_CONTROLLER_H
#define LAKOT_SKILL_WINDOW_RML_CONTROLLER_H

#include <cstdint>
#include <functional>
#include <vector>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>

#include "ClassCatalog.h"
#include "SkillCatalog.h"

#include "../../network/GameTypes.h"

namespace lakot
{

class Localization;
class RmlUiLayer;

class SkillWindowRmlController
{
public:
    using RaiseSkillCallback = std::function<void(uint32_t pSkillId)>;
    using ChoosePathCallback = std::function<void(uint32_t pPathId)>;

    SkillWindowRmlController(RmlUiLayer& pRmlUiLayer, const Localization& pLocalization, const SkillCatalog& pSkillCatalog, const ClassCatalog& pClassCatalog);
    ~SkillWindowRmlController();

    SkillWindowRmlController(const SkillWindowRmlController&) = delete;
    SkillWindowRmlController& operator=(const SkillWindowRmlController&) = delete;

    void setStats(const CharacterStats& pStats);

    void setRaiseSkillCallback(RaiseSkillCallback pCallback);
    void setChoosePathCallback(ChoosePathCallback pCallback);

    void toggle();
    void close();
    bool isOpen() const;

private:
    struct PathRow
    {
        int id = 0;
        Rml::String name;
        Rml::String skillNames;
        Rml::String warningText;
        bool isPending = false;
    };

    struct SkillRow
    {
        int id = 0;
        Rml::String name;
        Rml::String description;
        Rml::String icon;
        Rml::String levelText;
        bool isLearned = false;
        bool isLocked = false;
        Rml::String lockText;
        bool canRaise = false;
        Rml::String statsText;
        Rml::String tierText;
    };

    RmlUiLayer& mRmlUiLayer;
    const Localization& mLocalization;
    const SkillCatalog& mSkillCatalog;
    const ClassCatalog& mClassCatalog;

    Rml::ElementDocument* mDocument{nullptr};
    Rml::DataModelHandle mModelHandle;

    CharacterStats mStats;

    Rml::String mClassName;
    Rml::String mPathName;
    int mSkillPoints{0};
    bool mHasPath{false};
    bool mCanChoosePath{false};
    bool mHasSkillPoints{false};
    int mPendingPathId{0};

    std::vector<PathRow> mPaths;
    std::vector<SkillRow> mSkills;

    bool mIsOpen{false};

    RaiseSkillCallback mRaiseSkillCallback;
    ChoosePathCallback mChoosePathCallback;

    void onCloseClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onRaiseSkillClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onChoosePathClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onConfirmPathClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);
    void onCancelPathClicked(Rml::DataModelHandle pHandle, Rml::Event& pEvent, const Rml::VariantList& pArguments);

    void rebuild();
    SkillRow makeSkillRow(const SkillDefinition& pSkill) const;
    Rml::String formatCooldown(float pSeconds) const;
};

}

#endif
