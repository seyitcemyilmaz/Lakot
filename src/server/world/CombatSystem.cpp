#include "CombatSystem.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace lakot;

CombatSystem::CombatSystem(const MapData& pMap, WorldRegistry& pRegistry,
                           std::unordered_map<uint64_t, Character>& pCharacters,
                           std::unordered_map<uint64_t, Monster>& pMonsters)
    : mMap(pMap)
    , mRegistry(pRegistry)
    , mCharacters(pCharacters)
    , mMonsters(pMonsters)
{

}

bool CombatSystem::canBeTargeted(uint64_t pEntityId) const
{
    const PlayerState* tState = mRegistry.getState(pEntityId);

    if (!tState || mMap.isSafe(tState->x, tState->z) || isDead(pEntityId))
    {
        return false;
    }

    return mCharacters.count(pEntityId) != 0 || mMonsters.count(pEntityId) != 0;
}

bool CombatSystem::isDead(uint64_t pEntityId) const
{
    if (auto tCharacter = mCharacters.find(pEntityId); tCharacter != mCharacters.end())
    {
        return tCharacter->second.isDead();
    }

    if (auto tMonster = mMonsters.find(pEntityId); tMonster != mMonsters.end())
    {
        return tMonster->second.state == MonsterStateType::eDead;
    }

    return false;
}

uint32_t CombatSystem::getHealthPercent(uint64_t pEntityId) const
{
    if (auto tCharacter = mCharacters.find(pEntityId); tCharacter != mCharacters.end())
    {
        int32_t tMax = std::max(1, tCharacter->second.getMaxHealth());
        return static_cast<uint32_t>(std::clamp(tCharacter->second.getStats().health * 100 / tMax, 0, 100));
    }

    if (auto tMonster = mMonsters.find(pEntityId); tMonster != mMonsters.end())
    {
        int32_t tMax = std::max(1, tMonster->second.stats.maxHealth);
        return static_cast<uint32_t>(std::clamp(tMonster->second.health * 100 / tMax, 0, 100));
    }

    return 100;
}

void CombatSystem::markVitalsChanged(uint64_t pEntityId)
{
    mChangedVitals.insert(pEntityId);
}

bool CombatSystem::isHostile(uint64_t pAttackerId, uint64_t pTargetId) const
{
    auto tAttackerCharacter = mCharacters.find(pAttackerId);
    auto tTargetCharacter = mCharacters.find(pTargetId);

    if (tAttackerCharacter != mCharacters.end())
    {
        if (tTargetCharacter == mCharacters.end())
        {
            return mMonsters.count(pTargetId) != 0;
        }

        uint32_t tAttackerKingdom = tAttackerCharacter->second.getKingdom();
        uint32_t tTargetKingdom = tTargetCharacter->second.getKingdom();
        return tAttackerKingdom != 0 && tTargetKingdom != 0 && tAttackerKingdom != tTargetKingdom;
    }

    return mMonsters.count(pAttackerId) != 0 && tTargetCharacter != mCharacters.end();
}

int32_t CombatSystem::getDefense(uint64_t pEntityId) const
{
    if (auto tCharacter = mCharacters.find(pEntityId); tCharacter != mCharacters.end())
    {
        return tCharacter->second.getDefense();
    }

    if (auto tMonster = mMonsters.find(pEntityId); tMonster != mMonsters.end())
    {
        return tMonster->second.stats.defense;
    }

    return 0;
}

CombatHit CombatSystem::applyHit(uint64_t pAttackerId, int32_t pAttack, uint64_t pTargetId, double pNow)
{
    std::uniform_real_distribution<float> tRoll(0.9f, 1.1f);
    int32_t tDamage = StatFormula::getDamage(pAttack, getDefense(pTargetId), tRoll(mRandom));

    CombatHit tHit;
    tHit.targetId = pTargetId;
    tHit.damage = static_cast<uint32_t>(tDamage);
    mChangedVitals.insert(pTargetId);

    if (auto tCharacter = mCharacters.find(pTargetId); tCharacter != mCharacters.end())
    {
        tHit.killed = tCharacter->second.takeDamage(tDamage);

        if (tHit.killed)
        {
            mRespawnAt[pTargetId] = pNow + kPlayerRespawnSeconds;
        }

        return tHit;
    }

    Monster& tMonster = mMonsters.at(pTargetId);
    tMonster.health = std::max(0, tMonster.health - tDamage);

    if (tMonster.state == MonsterStateType::eIdle || tMonster.state == MonsterStateType::eReturn)
    {
        tMonster.state = MonsterStateType::eChase;
        tMonster.targetId = pAttackerId;
        reportAggro(tMonster.id, pAttackerId);
    }

    if (tMonster.health == 0)
    {
        tHit.killed = true;
        tMonster.state = MonsterStateType::eDead;
        tMonster.targetId = 0;
        tMonster.removeAt = pNow + kCorpseSeconds;
        tMonster.respawnAt = pNow + tMonster.monsterTemplate->respawnSeconds;

        if (auto tKiller = mCharacters.find(pAttackerId); tKiller != mCharacters.end())
        {
            CombatReward tReward;
            tReward.characterId = pAttackerId;
            tReward.monsterTemplateId = tMonster.monsterTemplate->id;
            tReward.monsterEntityId = tMonster.id;
            tReward.level = tMonster.stats.level;
            tReward.levelsGained = tKiller->second.addExperience(tMonster.stats.experience);
            tReward.newLevel = tKiller->second.getStats().level;
            if (const PlayerState* tMonsterState = mRegistry.getState(pTargetId))
            {
                tReward.x = tMonsterState->x;
                tReward.z = tMonsterState->z;
            }

            mRewards.push_back(tReward);
        }
    }

    return tHit;
}

bool CombatSystem::playerAttack(uint64_t pAttackerId, float pYaw, uint32_t pCombo, double pNow)
{
    auto tAttacker = mCharacters.find(pAttackerId);
    const PlayerState* tState = mRegistry.getState(pAttackerId);

    if (tAttacker == mCharacters.end() || !tState || tAttacker->second.isDead() || mMap.isSafe(tState->x, tState->z))
    {
        return false;
    }

    double& tNextAttackAt = mNextAttackAt[pAttackerId];

    if (pNow + kAttackIntervalTolerance < tNextAttackAt)
    {
        return false;
    }

    tNextAttackAt = pNow + kPlayerAttackInterval;

    CombatRecord tRecord;
    tRecord.attackerId = pAttackerId;
    tRecord.combo = pCombo;

    float tForwardX = std::sin(pYaw);
    float tForwardZ = std::cos(pYaw);
    float tReach = kPlayerAttackRange + kBodyRadius;

    mRegistry.queryRadius(tState->x, tState->z, tReach, mCandidates);

    for (uint64_t tTargetId : mCandidates)
    {
        if (tTargetId == pAttackerId || !canBeTargeted(tTargetId) || !isHostile(pAttackerId, tTargetId))
        {
            continue;
        }

        const PlayerState* tTargetState = mRegistry.getState(tTargetId);
        float tDx = tTargetState->x - tState->x;
        float tDz = tTargetState->z - tState->z;
        float tDistance = std::sqrt(tDx * tDx + tDz * tDz);

        if (tDistance > kBodyRadius && (tDx * tForwardX + tDz * tForwardZ) / tDistance < kAttackArcCos)
        {
            continue;
        }

        tRecord.hits.push_back(applyHit(pAttackerId, tAttacker->second.getAttack(), tTargetId, pNow));
    }

    mRecords.push_back(std::move(tRecord));
    return true;
}

void CombatSystem::monsterAttack(Monster& pMonster, uint64_t pTargetId, double pNow)
{
    CombatRecord tRecord;
    tRecord.attackerId = pMonster.id;
    tRecord.targetId = pTargetId;
    tRecord.projectile = pMonster.monsterTemplate->projectile;

    if (canBeTargeted(pTargetId) && isHostile(pMonster.id, pTargetId))
    {
        tRecord.hits.push_back(applyHit(pMonster.id, pMonster.stats.attack, pTargetId, pNow));
    }

    mRecords.push_back(std::move(tRecord));
}

std::vector<uint64_t> CombatSystem::reviveDuePlayers(double pNow)
{
    std::vector<uint64_t> tRevived;

    for (auto tIterator = mRespawnAt.begin(); tIterator != mRespawnAt.end();)
    {
        auto tCharacter = mCharacters.find(tIterator->first);

        if (tCharacter == mCharacters.end())
        {
            tIterator = mRespawnAt.erase(tIterator);
            continue;
        }

        if (pNow < tIterator->second)
        {
            ++tIterator;
            continue;
        }

        tCharacter->second.revive();
        mChangedVitals.insert(tIterator->first);
        tRevived.push_back(tIterator->first);
        tIterator = mRespawnAt.erase(tIterator);
    }

    return tRevived;
}

double CombatSystem::getRespawnRemaining(uint64_t pCharacterId, double pNow) const
{
    auto tIterator = mRespawnAt.find(pCharacterId);
    return tIterator == mRespawnAt.end() ? 0.0 : std::max(0.0, tIterator->second - pNow);
}

void CombatSystem::forget(uint64_t pCharacterId)
{
    mNextAttackAt.erase(pCharacterId);
    mRespawnAt.erase(pCharacterId);
    mChangedVitals.erase(pCharacterId);
}

std::vector<CombatRecord> CombatSystem::takeRecords()
{
    return std::exchange(mRecords, {});
}

std::unordered_set<uint64_t> CombatSystem::takeChangedVitals()
{
    return std::exchange(mChangedVitals, {});
}

std::vector<CombatReward> CombatSystem::takeRewards()
{
    return std::exchange(mRewards, {});
}

std::vector<MonsterAggro> CombatSystem::takeAggro()
{
    return std::exchange(mAggro, {});
}

void CombatSystem::reportAggro(uint64_t pMonsterId, uint64_t pTargetId)
{
    mAggro.push_back({pMonsterId, pTargetId});
}
