#ifndef LAKOT_SKILLCATALOG_H
#define LAKOT_SKILLCATALOG_H

#include <cstdint>
#include <string>
#include <vector>

#include "ClassCatalog.h"

namespace lakot
{

enum class SkillKindType
{
    eDamage,
    eHeal,
    eBuff,
    eDebuff,
    eMovement,
    eSummon
};

enum class SkillTargetingType
{
    eSelf,
    eSingle,
    eAlly,
    eCone,
    eArea
};

struct SkillDefinition
{
    uint32_t id = 0;
    std::string key;
    std::string name;
    std::string description;
    uint32_t classId = 0;
    uint32_t pathId = 0;
    uint32_t unlockLevel = 1;
    SkillKindType kind = SkillKindType::eDamage;
    SkillTargetingType targeting = SkillTargetingType::eSelf;
    float range = 0.0f;
    float cooldown = 0.0f;
    uint32_t mpCost = 0;

    bool isFixed() const;
};

class SkillCatalog
{
public:
    bool load(const std::string& pDataDirectory, const ClassCatalog& pClassCatalog, std::string& pError);

    const SkillDefinition* find(uint32_t pSkillId) const;
    std::vector<const SkillDefinition*> getForClass(uint32_t pClassId) const;
    uint32_t getMaxSkillLevel() const;
    uint32_t getSkillPointsPerLevel() const;

private:
    std::vector<SkillDefinition> mSkills;
    uint32_t mMaxSkillLevel = 0;
    uint32_t mSkillPointsPerLevel = 0;
};

}

#endif
