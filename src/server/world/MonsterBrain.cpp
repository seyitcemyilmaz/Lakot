#include "MonsterBrain.h"
#include "MovementRules.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

using namespace lakot;

MonsterBrain::MonsterBrain(const MapData& pMap, WorldRegistry& pRegistry,
                           std::unordered_map<uint64_t, Character>& pCharacters,
                           std::unordered_map<uint64_t, Monster>& pMonsters,
                           CombatSystem& pCombat)
    : mMap(pMap)
    , mRegistry(pRegistry)
    , mCharacters(pCharacters)
    , mMonsters(pMonsters)
    , mCombat(pCombat)
{

}

void MonsterBrain::place(Monster& pMonster)
{
    PlayerState tState;
    tState.x = pMonster.spawnX;
    tState.z = pMonster.spawnZ;
    tState.y = mMap.getHeightAt(tState.x, tState.z) + MovementRules::kBodyHalfHeight;

    rollStats(pMonster);
    pMonster.health = pMonster.stats.maxHealth;
    pMonster.state = MonsterStateType::eIdle;
    pMonster.targetId = 0;
    pMonster.isInWorld = true;

    mRegistry.add(pMonster.id, pMonster.monsterTemplate->name, tState, false);
}

void MonsterBrain::rollStats(Monster& pMonster)
{
    const MonsterTemplate& tTemplate = *pMonster.monsterTemplate;
    uint32_t tMin = pMonster.spawnMinLevel != 0 ? pMonster.spawnMinLevel : tTemplate.minLevel;
    uint32_t tMax = pMonster.spawnMaxLevel != 0 ? pMonster.spawnMaxLevel : tTemplate.maxLevel;
    if (pMonster.spawnMinLevel == 0 && pMonster.spawnMaxLevel != 0)
    {
        tMin = std::min(tMin, tMax);
    }

    tMax = std::max(tMin, tMax);

    std::uniform_int_distribution<uint32_t> tLevelRoll(tMin, tMax);
    uint32_t tLevel = tLevelRoll(mRandom);
    float tFactor = getMonsterLevelFactor(tTemplate, tLevel);

    MonsterStats& tStats = pMonster.stats;
    tStats.level = tLevel;
    tStats.maxHealth = std::max(1, static_cast<int32_t>(std::lround(tTemplate.maxHealth * tFactor)));
    tStats.attack = std::max(1, static_cast<int32_t>(std::lround(tTemplate.attack * tFactor)));
    tStats.defense = std::max(0, static_cast<int32_t>(std::lround(tTemplate.defense * tFactor)));
    tStats.experience = static_cast<uint32_t>(std::lround(tTemplate.experience * tFactor));
    tStats.goldFactor = tFactor;
}

void MonsterBrain::update(double pNow, float pDeltaSeconds)
{
    for (auto& [tId, tMonster] : mMonsters)
    {
        if (tMonster.state == MonsterStateType::eDead)
        {
            if (tMonster.isInWorld && pNow >= tMonster.removeAt)
            {
                mRegistry.remove(tId);
                tMonster.isInWorld = false;
            }

            if (!tMonster.isInWorld && pNow >= tMonster.respawnAt)
            {
                place(tMonster);
            }

            continue;
        }

        think(tMonster, pNow, pDeltaSeconds);
    }

    spreadAggro();
}

void MonsterBrain::spreadAggro()
{
    std::vector<MonsterAggro> tEvents = mCombat.takeAggro();

    if (tEvents.empty())
    {
        return;
    }

    std::unordered_set<uint64_t> tQueried;

    for (const MonsterAggro& tEvent : tEvents)
    {
        auto tSource = mMonsters.find(tEvent.monsterId);

        if (tSource == mMonsters.end() || tSource->second.state == MonsterStateType::eDead || tSource->second.monsterTemplate->assistRange <= 0.0f ||
            !tQueried.insert(tEvent.monsterId).second || !mCombat.canBeTargeted(tEvent.targetId))
        {
            continue;
        }

        const PlayerState* tSourceState = mRegistry.getState(tEvent.monsterId);

        if (!tSourceState)
        {
            continue;
        }

        mRegistry.queryRadius(tSourceState->x, tSourceState->z, tSource->second.monsterTemplate->assistRange, mAssisted);

        for (uint64_t tId : mAssisted)
        {
            auto tOther = mMonsters.find(tId);

            if (tOther == mMonsters.end() || tId == tEvent.monsterId || tOther->second.monsterTemplate != tSource->second.monsterTemplate ||
                tOther->second.state != MonsterStateType::eIdle)
            {
                continue;
            }

            tOther->second.state = MonsterStateType::eChase;
            tOther->second.targetId = tEvent.targetId;
        }
    }
}

uint64_t MonsterBrain::findTarget(const Monster& pMonster, const PlayerState& pState)
{
    mRegistry.queryRadius(pState.x, pState.z, pMonster.monsterTemplate->aggroRange, mCandidates);

    uint64_t tBest = 0;
    float tBestDistance = std::numeric_limits<float>::max();

    for (uint64_t tId : mCandidates)
    {
        if (mCharacters.count(tId) == 0 || !mCombat.canBeTargeted(tId))
        {
            continue;
        }

        const PlayerState* tOther = mRegistry.getState(tId);
        float tDistance = std::hypot(tOther->x - pState.x, tOther->z - pState.z);

        if (tDistance < tBestDistance)
        {
            tBest = tId;
            tBestDistance = tDistance;
        }
    }

    return tBest;
}

bool MonsterBrain::moveTowards(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep)
{
    float tDx = pX - pState.x;
    float tDz = pZ - pState.z;
    float tDistance = std::hypot(tDx, tDz);

    if (tDistance < 0.001f)
    {
        return false;
    }

    float tMove = std::min(pStep, tDistance);
    PlayerState tNext = pState;
    tNext.x += tDx / tDistance * tMove;
    tNext.z += tDz / tDistance * tMove;
    tNext.yaw = std::atan2(tDx, tDz);

    if (!mMap.isMoveAllowed(pState.x, pState.z, tNext.x, tNext.z) || mMap.isSafe(tNext.x, tNext.z))
    {
        return false;
    }

    tNext.y = mMap.getHeightAt(tNext.x, tNext.z) + MovementRules::kBodyHalfHeight;
    mRegistry.setState(pMonster.id, tNext);
    return true;
}

void MonsterBrain::think(Monster& pMonster, double pNow, float pDeltaSeconds)
{
    const PlayerState* tCurrent = mRegistry.getState(pMonster.id);

    if (!tCurrent)
    {
        return;
    }

    PlayerState tState = *tCurrent;
    const MonsterTemplate& tTemplate = *pMonster.monsterTemplate;
    float tStep = tTemplate.moveSpeed * pDeltaSeconds;
    float tFromSpawn = std::hypot(tState.x - pMonster.spawnX, tState.z - pMonster.spawnZ);

    if (pMonster.state == MonsterStateType::eIdle)
    {
        if (tTemplate.isAggressive)
        {
            if (uint64_t tTarget = findTarget(pMonster, tState))
            {
                pMonster.state = MonsterStateType::eChase;
                pMonster.targetId = tTarget;
                mCombat.reportAggro(pMonster.id, tTarget);
            }
        }

        return;
    }

    if (pMonster.state == MonsterStateType::eChase)
    {
        const PlayerState* tTarget = mRegistry.getState(pMonster.targetId);

        if (!tTarget || !mCombat.canBeTargeted(pMonster.targetId) || tFromSpawn > tTemplate.leashRange)
        {
            pMonster.state = MonsterStateType::eReturn;
            pMonster.targetId = 0;
            return;
        }

        float tDistance = std::hypot(tTarget->x - tState.x, tTarget->z - tState.z);

        if (tDistance <= tTemplate.attackRange + CombatSystem::kBodyRadius)
        {
            if (pNow >= pMonster.nextAttackAt)
            {
                pMonster.nextAttackAt = pNow + tTemplate.attackInterval;
                mCombat.monsterAttack(pMonster, pMonster.targetId, pNow);
            }

            return;
        }

        moveTowards(pMonster, tState, tTarget->x, tTarget->z, tStep);
        return;
    }

    if (pMonster.state == MonsterStateType::eReturn)
    {
        if (tFromSpawn <= kArriveDistance || !moveTowards(pMonster, tState, pMonster.spawnX, pMonster.spawnZ, tStep))
        {
            pMonster.state = MonsterStateType::eIdle;

            if (pMonster.health != pMonster.stats.maxHealth)
            {
                pMonster.health = pMonster.stats.maxHealth;
                mCombat.markVitalsChanged(pMonster.id);
            }
        }
    }
}
