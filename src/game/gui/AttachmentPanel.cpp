#include "AttachmentPanel.h"

#include "../scene/world/EquipmentAttachments.h"

using namespace lakot;

AttachmentPanel::AttachmentPanel(EquipmentAttachments& pAttachments, std::vector<std::string> pSavePaths)
    : Panel("Equipment Tuning (F10)", ImGuiWindowFlags_AlwaysAutoResize)
    , mAttachments(pAttachments)
    , mSavePaths(std::move(pSavePaths))
{
    isOpen = false;
}

void AttachmentPanel::render()
{
    bool tIsChanged = false;

    if (AttachmentRule* tWeapon = mAttachments.findRule(EquipSlotType::eWeapon))
    {
        ImGui::SeparatorText("Weapon");
        ImGui::PushID("weapon");
        tIsChanged |= ImGui::DragFloat3("Offset", &tWeapon->offset.x, 0.05f);
        tIsChanged |= ImGui::DragFloat3("Rotation", &tWeapon->rotationDegrees.x, 0.5f, -360.0f, 360.0f);
        tIsChanged |= ImGui::DragFloat("Length", &tWeapon->length, 0.5f, 1.0f, 1000.0f);
        tIsChanged |= ImGui::DragFloat("Grip", &tWeapon->grip, 0.002f, 0.0f, 1.0f);
        ImGui::PopID();
    }

    if (AttachmentRule* tHelmet = mAttachments.findRule(EquipSlotType::eHelmet))
    {
        ImGui::SeparatorText("Helmet");
        ImGui::PushID("helmet");
        tIsChanged |= ImGui::DragFloat3("Offset", &tHelmet->offset.x, 0.002f);
        tIsChanged |= ImGui::DragFloat3("Rotation", &tHelmet->rotationDegrees.x, 0.5f, -360.0f, 360.0f);
        tIsChanged |= ImGui::DragFloat("Width scale", &tHelmet->widthScale, 0.01f, 0.1f, 10.0f);
        ImGui::PopID();
    }

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

    if (ImGui::Button("Revert"))
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
