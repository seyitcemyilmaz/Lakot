#ifndef LAKOT_ATTACHMENT_PANEL_H
#define LAKOT_ATTACHMENT_PANEL_H

#include <string>
#include <vector>

#include "Panel.h"

namespace lakot
{

class EquipmentAttachments;

struct AttachmentPanel : public Panel
{
    AttachmentPanel(EquipmentAttachments& pAttachments, std::vector<std::string> pSavePaths);

    void render() override;

private:
    EquipmentAttachments& mAttachments;
    std::vector<std::string> mSavePaths;
    std::string mStatus;
};

}

#endif
