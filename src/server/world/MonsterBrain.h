#ifndef LAKOT_SERVER_MONSTERBRAIN_H
#define LAKOT_SERVER_MONSTERBRAIN_H

#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

#include "CombatSystem.h"
#include "Monster.h"
#include "MapData.h"
#include "WorldRegistry.h"

#include "../game/Character.h"

namespace lakot
{

class MonsterBrain
{
public:
    MonsterBrain(const MapData& pMap, WorldRegistry& pRegistry,
                 std::unordered_map<uint64_t, Character>& pCharacters,
                 std::unordered_map<uint64_t, Monster>& pMonsters,
                 CombatSystem& pCombat);

    void place(Monster& pMonster);
    void update(double pNow, float pDeltaSeconds);

private:
    static constexpr float kArriveDistance = 0.8f;
    static constexpr int kMaxPathSearchesPerTick = 4;
    static constexpr int kMaxPathExpansions = 2500;
    static constexpr double kPathSearchIntervalSeconds = 0.5;
    static constexpr double kChaseStuckSeconds = 4.0;
    static constexpr double kReturnStuckSeconds = 1.5;

    const MapData& mMap;
    WorldRegistry& mRegistry;
    std::unordered_map<uint64_t, Character>& mCharacters;
    std::unordered_map<uint64_t, Monster>& mMonsters;
    CombatSystem& mCombat;

    std::mt19937 mRandom{std::random_device{}()};
    std::vector<uint64_t> mCandidates;
    std::vector<uint64_t> mAssisted;
    int mSearchesLeft = kMaxPathSearchesPerTick;

    void rollStats(Monster& pMonster);
    void spreadAggro();
    void think(Monster& pMonster, double pNow, float pDeltaSeconds);
    uint64_t findTarget(const Monster& pMonster, const PlayerState& pState);
    uint64_t findRetarget(Monster& pMonster);
    bool moveTowards(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep, double pNow);
    bool advance(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep, double pNow);
    bool stepOnce(const PlayerState& pFrom, float pX, float pZ, float pStep, PlayerState& pNext) const;
    bool stepStraight(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep);
    bool followPath(Monster& pMonster, const PlayerState& pState, float pStep);
    bool searchPath(Monster& pMonster, const PlayerState& pState, float pX, float pZ, double pNow);
    bool isStuckFor(const Monster& pMonster, double pNow, double pSeconds) const;
};

}

#endif
