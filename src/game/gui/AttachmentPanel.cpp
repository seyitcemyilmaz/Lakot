#include "AttachmentPanel.h"

#include "../item/EquipmentAttachments.h"

using namespace lakot;

AttachmentPanel::AttachmentPanel(EquipmentAttachments& pAttachments, std::vector<std::string> pSavePaths, EquippedModelCallback pEquippedModel)
    : Panel("Equipment Tuning (F10)", ImGuiWindowFlags_AlwaysAutoResize)
    , mAttachments(pAttachments)
    , mSavePaths(std::move(pSavePaths))
    , mEquippedModel(std::move(pEquippedModel))
{
    isOpen = false;
}

bool AttachmentPanel::renderWeaponFields(AttachmentRule& pRule)
{
    bool tIsChanged = false;
    tIsChanged |= ImGui::DragFloat3("Offset", &pRule.offset.x, 0.05f);
    tIsChanged |= ImGui::DragFloat3("Rotation", &pRule.rotationDegrees.x, 0.5f, -360.0f, 360.0f);
    tIsChanged |= ImGui::DragFloat("Length", &pRule.length, 0.5f, 1.0f, 1000.0f);
    tIsChanged |= ImGui::DragFloat("Grip", &pRule.grip, 0.002f, 0.0f, 1.0f);

    if (ImGui::Button("Flip 180"))
    {
        pRule.grip = 1.0f - pRule.grip;
        pRule.rotationDegrees = EquipmentAttachments::flipRotation(pRule.rotationDegrees);
        tIsChanged = true;
    }

    return tIsChanged;
}

bool AttachmentPanel::renderHelmetFields(AttachmentRule& pRule)
{
    bool tIsChanged = false;
    tIsChanged |= ImGui::DragFloat3("Offset", &pRule.offset.x, 0.002f);
    tIsChanged |= ImGui::DragFloat3("Rotation", &pRule.rotationDegrees.x, 0.5f, -360.0f, 360.0f);
    tIsChanged |= ImGui::DragFloat("Width scale", &pRule.widthScale, 0.01f, 0.1f, 10.0f);
    return tIsChanged;
}

bool AttachmentPanel::renderSlot(EquipSlotType pSlot, const char* pTitle)
{
    AttachmentRule* tGlobal = mAttachments.findRule(pSlot);

    if (!tGlobal)
    {
        return false;
    }

    ImGui::SeparatorText(pTitle);
    ImGui::PushID(pTitle);

    std::string tModelPath = mEquippedModel ? mEquippedModel(pSlot) : std::string();
    AttachmentRule* tRule = tGlobal;
    bool tIsChanged = false;

    if (mIsModelMode)
    {
        if (tModelPath.empty())
        {
            ImGui::TextUnformatted("Nothing equipped");
            ImGui::PopID();
            return false;
        }

        ImGui::TextUnformatted(tModelPath.c_str());
        tRule = mAttachments.findOverride(pSlot, tModelPath);

        if (!tRule)
        {
            ImGui::TextUnformatted("No override: global rule applies");

            if (ImGui::Button("Create override"))
            {
                mAttachments.createOverride(pSlot, tModelPath);
                tIsChanged = true;
            }

            ImGui::PopID();
            return tIsChanged;
        }
    }

    tIsChanged |= pSlot == EquipSlotType::eWeapon ? renderWeaponFields(*tRule) : renderHelmetFields(*tRule);

    if (mIsModelMode && ImGui::Button("Delete override"))
    {
        mAttachments.removeOverride(pSlot, tModelPath);
        tIsChanged = true;
    }

    ImGui::PopID();
    return tIsChanged;
}

void AttachmentPanel::render()
{
    if (ImGui::RadioButton("This model", mIsModelMode))
    {
        mIsModelMode = true;
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Global", !mIsModelMode))
    {
        mIsModelMode = false;
    }

    bool tIsChanged = renderSlot(EquipSlotType::eWeapon, "Weapon");
    tIsChanged |= renderSlot(EquipSlotType::eHelmet, "Helmet");

    if (tIsChanged)
    {
        mAttachments.invalidate();
        mStatus.clear();
    }

    ImGui::Separator();

    if (ImGui::Button("Save"))
    {
        mStatus = "Saved";

        for (const std::string& tPath : mSavePaths)
        {
            std::string tError;

            if (!mAttachments.save(tPath, tError))
            {
                mStatus = tError;
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Revert") && !mSavePaths.empty())
    {
        std::string tError;
        mStatus = mAttachments.load(mSavePaths.front(), tError) ? "Reverted" : tError;
    }

    if (!mStatus.empty())
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(mStatus.c_str());
    }
}
