#ifndef LAKOT_SERVER_MONSTER_H
#define LAKOT_SERVER_MONSTER_H

#include <cstdint>

#include "MonsterCatalog.h"

namespace lakot
{

enum class MonsterStateType
{
    eIdle,
    eChase,
    eReturn,
    eDead
};

struct MonsterStats
{
    uint32_t level = 1;
    int32_t maxHealth = 1;
    int32_t attack = 1;
    int32_t defense = 0;
    uint32_t experience = 0;
    float goldFactor = 1.0f;
};

inline float getMonsterLevelFactor(const MonsterTemplate& pTemplate, uint32_t pLevel)
{
    float tFactor = 1.0f + pTemplate.scalePerLevel * (static_cast<float>(pLevel) - static_cast<float>(pTemplate.level));
    return tFactor < 0.1f ? 0.1f : tFactor;
}

struct Monster
{
    uint64_t id = 0;
    const MonsterTemplate* monsterTemplate = nullptr;
    float spawnX = 0.0f;
    float spawnZ = 0.0f;
    MonsterStats stats;
    uint32_t spawnMinLevel = 0;
    uint32_t spawnMaxLevel = 0;
    int32_t health = 0;
    MonsterStateType state = MonsterStateType::eIdle;
    uint64_t targetId = 0;
    double nextAttackAt = 0.0;
    double removeAt = 0.0;
    double respawnAt = 0.0;
    bool isInWorld = true;
};

}

#endif
