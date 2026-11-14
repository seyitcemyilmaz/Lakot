#ifndef LAKOT_SERVER_MONSTER_H
#define LAKOT_SERVER_MONSTER_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

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

struct MonsterPath
{
    std::vector<glm::vec2> waypoints;
    size_t nextIndex = 0;
    MonsterStateType state = MonsterStateType::eIdle;
    uint64_t targetId = 0;
    glm::vec2 goal{0.0f, 0.0f};
    double nextSearchAt = 0.0;

    bool isActive() const
    {
        return nextIndex < waypoints.size();
    }

    void clear()
    {
        waypoints.clear();
        nextIndex = 0;
    }
};

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
    MonsterPath path;
    double stuckSince = -1.0;
    double nextAttackAt = 0.0;
    double removeAt = 0.0;
    double respawnAt = 0.0;
    bool isInWorld = true;
    bool isTemporary = false;
    std::unordered_map<uint64_t, uint32_t> damageBy;
};

}

#endif
