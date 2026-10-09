#ifndef LAKOT_MONSTERCATALOG_H
#define LAKOT_MONSTERCATALOG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace lakot
{

struct MonsterDrop
{
    uint32_t itemId = 0;
    float chance = 0.0f;
    uint32_t minCount = 1;
    uint32_t maxCount = 1;
    bool guaranteed = false;
};

struct MonsterTemplate
{
    uint32_t id = 0;
    std::string name;
    std::string model;
    uint32_t level = 1;
    uint32_t minLevel = 1;
    uint32_t maxLevel = 1;
    float scalePerLevel = 0.1f;
    int32_t maxHealth = 1;
    int32_t attack = 1;
    int32_t defense = 0;
    uint32_t experience = 0;
    float moveSpeed = 3.0f;
    float aggroRange = 8.0f;
    float attackRange = 1.8f;
    float attackInterval = 1.5f;
    float leashRange = 30.0f;
    float respawnSeconds = 15.0f;
    bool isAggressive = true;
    std::string projectile;
    float assistRange = 0.0f;
    bool isBoss = false;
    uint32_t lootRolls = 1;
    float tint[3] = {1.0f, 1.0f, 1.0f};
    float scale = 1.0f;
    float goldChance = 0.0f;
    uint32_t goldMin = 0;
    uint32_t goldMax = 0;
    std::vector<MonsterDrop> drops;
};

class MonsterCatalog
{
public:
    bool load(const std::string& pDataDirectory, std::string& pError);

    const MonsterTemplate* find(uint32_t pId) const;
    size_t getCount() const;
    const std::unordered_map<uint32_t, MonsterTemplate>& getAll() const;

private:
    std::unordered_map<uint32_t, MonsterTemplate> mTemplates;
};

}

#endif
