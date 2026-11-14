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

    const Placement* find(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem, const std::string& pModelPath);

    AttachmentRule* findRule(EquipSlotType pSlot);
    AttachmentRule* findOverride(EquipSlotType pSlot, const std::string& pModelPath);
    AttachmentRule* createOverride(EquipSlotType pSlot, const std::string& pModelPath);
    void removeOverride(EquipSlotType pSlot, const std::string& pModelPath);
    void invalidate();

    static glm::vec3 flipRotation(const glm::vec3& pDegrees);

private:
    using OverrideMap = std::map<std::string, AttachmentRule>;

    std::optional<AttachmentRule> mWeapon;
    std::optional<AttachmentRule> mHelmet;
    OverrideMap mWeaponOverrides;
    OverrideMap mHelmetOverrides;
    std::map<std::tuple<EquipSlotType, const SkinnedModel*, const SkinnedModel*>, std::optional<Placement>> mCache;

    OverrideMap* getOverrides(EquipSlotType pSlot);
    const AttachmentRule* resolveRule(EquipSlotType pSlot, const std::string& pModelPath) const;
    std::optional<Placement> compute(EquipSlotType pSlot, const SkinnedModel& pCharacter, const SkinnedModel& pItem, const std::string& pModelPath) const;
};

}

#endif
