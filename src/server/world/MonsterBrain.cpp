#include "MonsterBrain.h"
#include "MovementRules.h"
#include "Pathfinder.h"

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
    pMonster.damageBy.clear();
    pMonster.state = MonsterStateType::eIdle;
    pMonster.targetId = 0;
    pMonster.stuckSince = -1.0;
    pMonster.path.clear();
    pMonster.isInWorld = true;

    mRegistry.add(pMonster.id, pMonster.monsterTemplate->name, tState, false);
    mCombat.markVitalsChanged(pMonster.id);
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
    mSearchesLeft = kMaxPathSearchesPerTick;

    for (auto tIterator = mMonsters.begin(); tIterator != mMonsters.end();)
    {
        auto& [tId, tMonster] = *tIterator;

        if (tMonster.state == MonsterStateType::eDead)
        {
            if (tMonster.isTemporary)
            {
                if (pNow >= tMonster.removeAt)
                {
                    mRegistry.remove(tId);
                    tIterator = mMonsters.erase(tIterator);
                    continue;
                }

                ++tIterator;
                continue;
            }

            if (tMonster.isInWorld && pNow >= tMonster.removeAt)
            {
                mRegistry.remove(tId);
                tMonster.isInWorld = false;
            }

            if (!tMonster.isInWorld && pNow >= tMonster.respawnAt)
            {
                place(tMonster);
            }

            ++tIterator;
            continue;
        }

        think(tMonster, pNow, pDeltaSeconds);
        ++tIterator;
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

uint64_t MonsterBrain::findRetarget(Monster& pMonster)
{
    const MonsterTemplate& tTemplate = *pMonster.monsterTemplate;
    uint64_t tBest = 0;
    uint32_t tBestDamage = 0;

    for (auto tIterator = pMonster.damageBy.begin(); tIterator != pMonster.damageBy.end();)
    {
        uint64_t tId = tIterator->first;

        if (mCharacters.count(tId) == 0)
        {
            tIterator = pMonster.damageBy.erase(tIterator);
            continue;
        }

        const PlayerState* tOther = mRegistry.getState(tId);
        bool tIsInRange = tOther && std::hypot(tOther->x - pMonster.spawnX, tOther->z - pMonster.spawnZ) <= tTemplate.leashRange;

        if (tIsInRange && mCombat.canBeTargeted(tId) && (tBest == 0 || tIterator->second > tBestDamage || (tIterator->second == tBestDamage && tId < tBest)))
        {
            tBest = tId;
            tBestDamage = tIterator->second;
        }

        ++tIterator;
    }

    return tBest;
}

bool MonsterBrain::stepOnce(const PlayerState& pFrom, float pX, float pZ, float pStep, PlayerState& pNext) const
{
    float tDx = pX - pFrom.x;
    float tDz = pZ - pFrom.z;
    float tDistance = std::hypot(tDx, tDz);

    if (tDistance < 0.001f)
    {
        return false;
    }

    float tMove = std::min(pStep, tDistance);
    pNext = pFrom;
    pNext.x += tDx / tDistance * tMove;
    pNext.z += tDz / tDistance * tMove;
    pNext.yaw = std::atan2(tDx, tDz);

    if (!mMap.isMoveAllowed(pFrom.x, pFrom.z, pNext.x, pNext.z) || mMap.isSafe(pNext.x, pNext.z))
    {
        return false;
    }

    pNext.y = mMap.getHeightAt(pNext.x, pNext.z) + MovementRules::kBodyHalfHeight;
    return true;
}

bool MonsterBrain::stepStraight(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep)
{
    PlayerState tNext;

    if (!stepOnce(pState, pX, pZ, pStep, tNext))
    {
        return false;
    }

    mRegistry.setState(pMonster.id, tNext);
    return true;
}

bool MonsterBrain::followPath(Monster& pMonster, const PlayerState& pState, float pStep)
{
    MonsterPath& tPath = pMonster.path;
    PlayerState tCurrent = pState;
    float tRemaining = pStep;
    bool tHasMoved = false;

    while (tRemaining > 0.001f && tPath.isActive())
    {
        const glm::vec2& tWaypoint = tPath.waypoints[tPath.nextIndex];
        float tDistance = std::hypot(tWaypoint.x - tCurrent.x, tWaypoint.y - tCurrent.z);
        float tMove = std::min(tRemaining, tDistance);

        if (tDistance >= 0.001f)
        {
            PlayerState tNext;

            if (!stepOnce(tCurrent, tWaypoint.x, tWaypoint.y, tMove, tNext))
            {
                break;
            }

            tCurrent = tNext;
            tRemaining -= tMove;
            tHasMoved = true;
        }

        if (tMove >= tDistance - 0.001f)
        {
            ++tPath.nextIndex;
        }
    }

    if (tHasMoved)
    {
        mRegistry.setState(pMonster.id, tCurrent);
    }

    return tHasMoved;
}

bool MonsterBrain::searchPath(Monster& pMonster, const PlayerState& pState, float pX, float pZ, double pNow)
{
    MonsterPath& tPath = pMonster.path;

    if (mSearchesLeft <= 0)
    {
        return tPath.isActive();
    }

    --mSearchesLeft;
    tPath.nextSearchAt = pNow + kPathSearchIntervalSeconds;
    tPath.state = pMonster.state;
    tPath.targetId = pMonster.targetId;
    tPath.goal = glm::vec2(pX, pZ);
    tPath.nextIndex = 0;

    if (!Pathfinder::findBoundedPath(mMap, glm::vec2(pState.x, pState.z), tPath.goal, kMaxPathExpansions, tPath.waypoints))
    {
        tPath.clear();
        return false;
    }

    return true;
}

bool MonsterBrain::advance(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep, double pNow)
{
    MonsterPath& tPath = pMonster.path;

    if (tPath.isActive() && (tPath.state != pMonster.state || tPath.targetId != pMonster.targetId))
    {
        tPath.clear();
    }

    bool tIsFollowing = tPath.isActive();

    if (tIsFollowing && pNow >= tPath.nextSearchAt && glm::distance(tPath.goal, glm::vec2(pX, pZ)) > mMap.getCellSize())
    {
        tIsFollowing = searchPath(pMonster, pState, pX, pZ, pNow);
    }

    if (!tIsFollowing)
    {
        if (stepStraight(pMonster, pState, pX, pZ, pStep))
        {
            return true;
        }

        if (pNow < tPath.nextSearchAt || !searchPath(pMonster, pState, pX, pZ, pNow))
        {
            return false;
        }
    }

    if (followPath(pMonster, pState, pStep))
    {
        return true;
    }

    tPath.clear();
    return false;
}

bool MonsterBrain::moveTowards(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep, double pNow)
{
    bool tHasMoved = advance(pMonster, pState, pX, pZ, pStep, pNow);

    if (tHasMoved)
    {
        pMonster.stuckSince = -1.0;
    }
    else if (pMonster.stuckSince < 0.0)
    {
        pMonster.stuckSince = pNow;
    }

    return tHasMoved;
}

bool MonsterBrain::isStuckFor(const Monster& pMonster, double pNow, double pSeconds) const
{
    return pMonster.stuckSince >= 0.0 && pNow - pMonster.stuckSince >= pSeconds;
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

        if (tFromSpawn > tTemplate.leashRange)
        {
            pMonster.state = MonsterStateType::eReturn;
            pMonster.targetId = 0;
            return;
        }

        if (!tTarget || !mCombat.canBeTargeted(pMonster.targetId))
        {
            pMonster.targetId = findRetarget(pMonster);
            pMonster.path.clear();
            pMonster.stuckSince = -1.0;

            if (pMonster.targetId == 0)
            {
                pMonster.state = MonsterStateType::eReturn;
            }

            return;
        }

        float tDistance = std::hypot(tTarget->x - tState.x, tTarget->z - tState.z);

        if (tDistance <= tTemplate.attackRange + CombatRules::kBodyRadius)
        {
            pMonster.stuckSince = -1.0;

            if (pNow >= pMonster.nextAttackAt)
            {
                pMonster.nextAttackAt = pNow + tTemplate.attackInterval;
                mCombat.monsterAttack(pMonster, pMonster.targetId, pNow);
            }

            return;
        }

        if (!moveTowards(pMonster, tState, tTarget->x, tTarget->z, tStep, pNow) && isStuckFor(pMonster, pNow, kChaseStuckSeconds))
        {
            pMonster.state = MonsterStateType::eReturn;
            pMonster.targetId = 0;
            pMonster.stuckSince = -1.0;
            pMonster.path.clear();
        }

        return;
    }

    if (pMonster.state == MonsterStateType::eReturn)
    {
        if (tFromSpawn <= kArriveDistance || (!moveTowards(pMonster, tState, pMonster.spawnX, pMonster.spawnZ, tStep, pNow) && isStuckFor(pMonster, pNow, kReturnStuckSeconds)))
        {
            if (tFromSpawn > kArriveDistance)
            {
                PlayerState tHome = tState;
                tHome.x = pMonster.spawnX;
                tHome.z = pMonster.spawnZ;
                tHome.y = mMap.getHeightAt(tHome.x, tHome.z) + MovementRules::kBodyHalfHeight;
                mRegistry.setState(pMonster.id, tHome);
            }

            pMonster.state = MonsterStateType::eIdle;
            pMonster.stuckSince = -1.0;
            pMonster.path.clear();

            if (pMonster.health != pMonster.stats.maxHealth)
            {
                pMonster.health = pMonster.stats.maxHealth;
                pMonster.damageBy.clear();
                mCombat.markVitalsChanged(pMonster.id);
            }
        }
    }
}
