#ifndef LAKOT_EQUIPMENT_ATTACHMENTS_H
#define LAKOT_EQUIPMENT_ATTACHMENTS_H

#include <map>
#include <optional>
#include <string>
#include <tuple>

#include <glm/glm.hpp>

#include "../../network/GameTypes.h"

namespace lakot
{

class SkinnedModel;

struct AttachmentRule
{
    std::string joint;
    float length = 0.0f;
    float grip = 0.0f;
    float widthScale = 0.0f;
    glm::vec3 rotationDegrees{0.0f};
    glm::vec3 offset{0.0f};
};

class EquipmentAttachments
{
public:
    struct Placement
    {
        int joint;
        glm::mat4 bindTransform;
    };

    bool load(const std::string& pPath, std::string& pError);

    bool save(const std::string& pPath, std::string& pError) const;

    const Placement* find(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem);

    AttachmentRule* findRule(EquipSlotType pSlot);
    void invalidate();

private:
    std::optional<AttachmentRule> mWeapon;
    std::optional<AttachmentRule> mHelmet;
    std::map<std::tuple<EquipSlotType, const SkinnedModel*, const SkinnedModel*>, std::optional<Placement>> mCache;

    std::optional<Placement> compute(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem) const;
};

}

#endif
