#ifndef LAKOT_SERVER_COMBATSYSTEM_H
#define LAKOT_SERVER_COMBATSYSTEM_H

#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CombatRules.h"
#include "Monster.h"
#include "MapData.h"
#include "WorldRegistry.h"

#include "../game/Character.h"

namespace lakot
{

struct CombatHit
{
    uint64_t targetId = 0;
    uint32_t damage = 0;
    bool killed = false;
};

struct CombatRecord
{
    uint64_t attackerId = 0;
    uint32_t combo = 0;
    uint64_t targetId = 0;
    std::string projectile;
    std::vector<CombatHit> hits;
};

struct MonsterAggro
{
    uint64_t monsterId = 0;
    uint64_t targetId = 0;
};

struct DeathPenalty
{
    uint64_t characterId = 0;
    uint64_t experienceLost = 0;
};

enum class RespawnPlaceType
{
    eHere,
    eCity
};

enum class RespawnOutcomeType
{
    eOk,
    eNotDead,
    eTooEarly
};

struct CombatReward
{
    uint64_t characterId = 0;
    uint32_t monsterTemplateId = 0;
    uint32_t levelsGained = 0;
    uint32_t newLevel = 0;
    uint64_t monsterEntityId = 0;
    uint32_t level = 1;
    float x = 0.0f;
    float z = 0.0f;
    bool isLootOwner = false;
};

class CombatSystem
{
public:
    static constexpr double kPlayerRespawnSeconds = 5.0;
    static constexpr double kAutoRespawnSeconds = 60.0;
    static constexpr double kCorpseSeconds = 3.0;
    static constexpr float kMinRewardDamageShare = 0.05f;
    static constexpr double kRewardExperienceSwing = 0.05;

    CombatSystem(const MapData& pMap, WorldRegistry& pRegistry,
                 std::unordered_map<uint64_t, Character>& pCharacters,
                 std::unordered_map<uint64_t, Monster>& pMonsters);

    bool playerAttack(uint64_t pAttackerId, float pYaw, uint32_t pCombo, double pNow);
    void monsterAttack(Monster& pMonster, uint64_t pTargetId, double pNow);

    bool killPlayer(uint64_t pCharacterId, double pNow);
    bool revivePlayer(uint64_t pCharacterId);

    void scheduleRespawnIfDead(uint64_t pCharacterId, double pNow);
    std::vector<uint64_t> reviveDuePlayers(double pNow);
    double getRespawnRemaining(uint64_t pCharacterId, double pNow) const;
    double getAutoRespawnRemaining(uint64_t pCharacterId, double pNow) const;
    RespawnOutcomeType tryRespawn(uint64_t pCharacterId, RespawnPlaceType pPlace, double pNow);
    void forget(uint64_t pCharacterId);

    bool canBeTargeted(uint64_t pEntityId) const;
    bool isDead(uint64_t pEntityId) const;
    bool isInCombat(uint64_t pCharacterId, double pNow) const;
    uint32_t getHealthPercent(uint64_t pEntityId) const;
    void markVitalsChanged(uint64_t pEntityId);

    std::vector<CombatRecord> takeRecords();
    std::unordered_set<uint64_t> takeChangedVitals();
    std::vector<CombatReward> takeRewards();
    std::vector<DeathPenalty> takeDeathPenalties();
    std::vector<MonsterAggro> takeAggro();
    void reportAggro(uint64_t pMonsterId, uint64_t pTargetId);

private:
    const MapData& mMap;
    WorldRegistry& mRegistry;
    std::unordered_map<uint64_t, Character>& mCharacters;
    std::unordered_map<uint64_t, Monster>& mMonsters;

    std::mt19937 mRandom{std::random_device{}()};
    std::unordered_map<uint64_t, double> mNextAttackAt;
    struct DeathEntry
    {
        double choosableAt = 0.0;
        double autoAt = 0.0;
    };

    std::unordered_map<uint64_t, DeathEntry> mDeaths;
    double mAutoRespawnSeconds;
    std::unordered_map<uint64_t, double> mLastCombatAt;

    std::vector<CombatRecord> mRecords;
    std::unordered_set<uint64_t> mChangedVitals;
    std::vector<CombatReward> mRewards;
    std::vector<DeathPenalty> mDeathPenalties;
    std::vector<MonsterAggro> mAggro;
    std::vector<uint64_t> mCandidates;

    bool isHostile(uint64_t pAttackerId, uint64_t pTargetId) const;
    int32_t getDefense(uint64_t pEntityId) const;
    DeathEntry makeDeathEntry(double pNow) const;
    CombatHit applyHit(uint64_t pAttackerId, int32_t pAttack, uint64_t pTargetId, double pNow);
    void rewardKill(Monster& pMonster, uint64_t pKillerId);
};

}

#endif
