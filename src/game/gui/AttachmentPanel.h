#ifndef LAKOT_ATTACHMENT_PANEL_H
#define LAKOT_ATTACHMENT_PANEL_H

#include <functional>
#include <string>
#include <vector>

#include "../network/GameTypes.h"
#include "Panel.h"

namespace lakot
{

class EquipmentAttachments;
struct AttachmentRule;

struct AttachmentPanel : public Panel
{
    using EquippedModelCallback = std::function<std::string(EquipSlotType)>;

    AttachmentPanel(EquipmentAttachments& pAttachments, std::vector<std::string> pSavePaths, EquippedModelCallback pEquippedModel);

    void render() override;

private:
    EquipmentAttachments& mAttachments;
    std::vector<std::string> mSavePaths;
    EquippedModelCallback mEquippedModel;
    std::string mStatus;
    bool mIsModelMode = true;

    bool renderSlot(EquipSlotType pSlot, const char* pTitle);
    bool renderWeaponFields(AttachmentRule& pRule);
    bool renderHelmetFields(AttachmentRule& pRule);
};

}

#endif
