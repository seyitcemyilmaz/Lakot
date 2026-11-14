#include "CombatSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

using namespace lakot;

namespace
{
    constexpr double kMaxAutoRespawnSeconds = 3600.0;

    double readAutoRespawnSeconds()
    {
        const char* tValue = std::getenv("LAKOT_AUTO_RESPAWN_SECONDS");
        double tParsed = tValue != nullptr ? std::atof(tValue) : 0.0;

        return std::isfinite(tParsed) && tParsed > 0.0 ? std::min(tParsed, kMaxAutoRespawnSeconds) : CombatSystem::kAutoRespawnSeconds;
    }
}

CombatSystem::CombatSystem(const MapData& pMap, WorldRegistry& pRegistry,
                           std::unordered_map<uint64_t, Character>& pCharacters,
                           std::unordered_map<uint64_t, Monster>& pMonsters)
    : mMap(pMap)
    , mRegistry(pRegistry)
    , mCharacters(pCharacters)
    , mMonsters(pMonsters)
    , mAutoRespawnSeconds(readAutoRespawnSeconds())
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

    if (mCharacters.count(pAttackerId) != 0)
    {
        mLastCombatAt[pAttackerId] = pNow;
    }

    if (mCharacters.count(pTargetId) != 0)
    {
        mLastCombatAt[pTargetId] = pNow;
    }

    if (auto tCharacter = mCharacters.find(pTargetId); tCharacter != mCharacters.end())
    {
        tHit.killed = tCharacter->second.takeDamage(tDamage);

        if (tHit.killed)
        {
            mDeaths[pTargetId] = makeDeathEntry(pNow);

            if (uint64_t tLost = tCharacter->second.loseDeathExperience(); tLost > 0)
            {
                mDeathPenalties.push_back({pTargetId, tLost});
            }
        }

        return tHit;
    }

    Monster& tMonster = mMonsters.at(pTargetId);

    if (mCharacters.count(pAttackerId) != 0)
    {
        tMonster.damageBy[pAttackerId] += static_cast<uint32_t>(std::min(tDamage, tMonster.health));
    }

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

        rewardKill(tMonster, pAttackerId);
    }

    return tHit;
}

void CombatSystem::rewardKill(Monster& pMonster, uint64_t pKillerId)
{
    uint32_t tMinDamage = static_cast<uint32_t>(std::ceil(static_cast<float>(pMonster.stats.maxHealth) * kMinRewardDamageShare));
    std::vector<std::pair<uint64_t, uint32_t>> tEligible;
    uint64_t tTotalDamage = 0;

    for (const auto& [tCharacterId, tDamage] : pMonster.damageBy)
    {
        auto tCharacter = mCharacters.find(tCharacterId);

        if (tCharacter == mCharacters.end() || tCharacter->second.isDead() || (tDamage < tMinDamage && tCharacterId != pKillerId))
        {
            continue;
        }

        tEligible.emplace_back(tCharacterId, std::max<uint32_t>(tDamage, 1));
        tTotalDamage += std::max<uint32_t>(tDamage, 1);
    }

    pMonster.damageBy.clear();

    if (tEligible.empty())
    {
        return;
    }

    std::uniform_real_distribution<double> tExperienceSwing(1.0 - kRewardExperienceSwing, 1.0 + kRewardExperienceSwing);
    std::uniform_int_distribution<uint64_t> tLootRoll(0, tTotalDamage - 1);
    uint64_t tLootTicket = tLootRoll(mRandom);
    const PlayerState* tMonsterState = mRegistry.getState(pMonster.id);

    for (const auto& [tCharacterId, tDamage] : tEligible)
    {
        Character& tCharacter = mCharacters.at(tCharacterId);
        double tShare = static_cast<double>(tDamage) / static_cast<double>(tTotalDamage);
        uint64_t tExperience = static_cast<uint64_t>(std::llround(pMonster.stats.experience * tShare * tExperienceSwing(mRandom)));

        CombatReward tReward;
        tReward.characterId = tCharacterId;
        tReward.monsterTemplateId = pMonster.monsterTemplate->id;
        tReward.monsterEntityId = pMonster.id;
        tReward.level = pMonster.stats.level;
        tReward.levelsGained = tCharacter.addExperience(std::max<uint64_t>(tExperience, pMonster.stats.experience > 0 ? 1 : 0));
        tReward.newLevel = tCharacter.getStats().level;
        tReward.isLootOwner = tLootTicket < tDamage;
        tLootTicket = tReward.isLootOwner ? UINT64_MAX : tLootTicket - tDamage;

        if (tMonsterState)
        {
            tReward.x = tMonsterState->x;
            tReward.z = tMonsterState->z;
        }

        mRewards.push_back(tReward);
    }
}

bool CombatSystem::playerAttack(uint64_t pAttackerId, float pYaw, uint32_t pCombo, double pNow)
{
    auto tAttacker = mCharacters.find(pAttackerId);
    const PlayerState* tState = mRegistry.getState(pAttackerId);

    if (!std::isfinite(pYaw) || tAttacker == mCharacters.end() || !tState || tAttacker->second.isDead() || mMap.isSafe(tState->x, tState->z))
    {
        return false;
    }

    double& tNextAttackAt = mNextAttackAt[pAttackerId];

    if (pNow + CombatRules::kAttackIntervalTolerance < tNextAttackAt)
    {
        return false;
    }

    tNextAttackAt = std::max(tNextAttackAt, pNow) + CombatRules::kPlayerAttackInterval;

    CombatRecord tRecord;
    tRecord.attackerId = pAttackerId;
    tRecord.combo = pCombo % CombatRules::kComboCount;

    float tForwardX = std::sin(pYaw);
    float tForwardZ = std::cos(pYaw);
    float tReach = CombatRules::kPlayerAttackRange + CombatRules::kBodyRadius;

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

        if (tDistance > CombatRules::kBodyRadius && (tDx * tForwardX + tDz * tForwardZ) / tDistance < CombatRules::kAttackArcCos)
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

bool CombatSystem::killPlayer(uint64_t pCharacterId, double pNow)
{
    auto tCharacter = mCharacters.find(pCharacterId);

    if (tCharacter == mCharacters.end() || tCharacter->second.isDead())
    {
        return false;
    }

    tCharacter->second.takeDamage(tCharacter->second.getStats().health);
    mDeaths[pCharacterId] = makeDeathEntry(pNow);
    mChangedVitals.insert(pCharacterId);
    return true;
}

bool CombatSystem::revivePlayer(uint64_t pCharacterId)
{
    auto tCharacter = mCharacters.find(pCharacterId);

    if (tCharacter == mCharacters.end() || !tCharacter->second.isDead())
    {
        return false;
    }

    tCharacter->second.revive();
    mDeaths.erase(pCharacterId);
    mChangedVitals.insert(pCharacterId);
    return true;
}

void CombatSystem::scheduleRespawnIfDead(uint64_t pCharacterId, double pNow)
{
    auto tCharacter = mCharacters.find(pCharacterId);

    if (tCharacter != mCharacters.end() && tCharacter->second.isDead())
    {
        mDeaths.try_emplace(pCharacterId, makeDeathEntry(pNow));
    }
}

std::vector<uint64_t> CombatSystem::reviveDuePlayers(double pNow)
{
    std::vector<uint64_t> tRevived;

    for (auto tIterator = mDeaths.begin(); tIterator != mDeaths.end();)
    {
        auto tCharacter = mCharacters.find(tIterator->first);

        if (tCharacter == mCharacters.end())
        {
            tIterator = mDeaths.erase(tIterator);
            continue;
        }

        if (pNow < tIterator->second.autoAt)
        {
            ++tIterator;
            continue;
        }

        tCharacter->second.revive(StatFormula::kReviveCityHealthFraction);
        mChangedVitals.insert(tIterator->first);
        tRevived.push_back(tIterator->first);
        tIterator = mDeaths.erase(tIterator);
    }

    return tRevived;
}

double CombatSystem::getRespawnRemaining(uint64_t pCharacterId, double pNow) const
{
    auto tIterator = mDeaths.find(pCharacterId);
    return tIterator == mDeaths.end() ? 0.0 : std::max(0.0, tIterator->second.choosableAt - pNow);
}

double CombatSystem::getAutoRespawnRemaining(uint64_t pCharacterId, double pNow) const
{
    auto tIterator = mDeaths.find(pCharacterId);
    return tIterator == mDeaths.end() ? 0.0 : std::max(0.0, tIterator->second.autoAt - pNow);
}

RespawnOutcomeType CombatSystem::tryRespawn(uint64_t pCharacterId, RespawnPlaceType pPlace, double pNow)
{
    auto tCharacter = mCharacters.find(pCharacterId);
    auto tDeath = mDeaths.find(pCharacterId);

    if (tCharacter == mCharacters.end() || tDeath == mDeaths.end() || !tCharacter->second.isDead())
    {
        return RespawnOutcomeType::eNotDead;
    }

    if (pNow < tDeath->second.choosableAt)
    {
        return RespawnOutcomeType::eTooEarly;
    }

    tCharacter->second.revive(pPlace == RespawnPlaceType::eHere ? StatFormula::kReviveHereHealthFraction : StatFormula::kReviveCityHealthFraction);
    mDeaths.erase(tDeath);
    mChangedVitals.insert(pCharacterId);
    return RespawnOutcomeType::eOk;
}

CombatSystem::DeathEntry CombatSystem::makeDeathEntry(double pNow) const
{
    DeathEntry tEntry;
    tEntry.choosableAt = pNow + kPlayerRespawnSeconds;
    tEntry.autoAt = pNow + std::max(kPlayerRespawnSeconds, mAutoRespawnSeconds);
    return tEntry;
}

void CombatSystem::forget(uint64_t pCharacterId)
{
    mNextAttackAt.erase(pCharacterId);
    mDeaths.erase(pCharacterId);
    mChangedVitals.erase(pCharacterId);
    mLastCombatAt.erase(pCharacterId);
}

bool CombatSystem::isInCombat(uint64_t pCharacterId, double pNow) const
{
    auto tIterator = mLastCombatAt.find(pCharacterId);
    return tIterator != mLastCombatAt.end() && pNow - tIterator->second < StatFormula::kCombatWindowSeconds;
}

std::vector<CombatRecord> CombatSystem::takeRecords()
{
    return std::exchange(mRecords, {});
}

std::unordered_set<uint64_t> CombatSystem::takeChangedVitals()
{
    return std::exchange(mChangedVitals, {});
}

std::vector<DeathPenalty> CombatSystem::takeDeathPenalties()
{
    return std::exchange(mDeathPenalties, {});
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
