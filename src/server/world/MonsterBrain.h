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

    const MapData& mMap;
    WorldRegistry& mRegistry;
    std::unordered_map<uint64_t, Character>& mCharacters;
    std::unordered_map<uint64_t, Monster>& mMonsters;
    CombatSystem& mCombat;

    std::mt19937 mRandom{std::random_device{}()};
    std::vector<uint64_t> mCandidates;
    std::vector<uint64_t> mAssisted;

    void rollStats(Monster& pMonster);
    void spreadAggro();
    void think(Monster& pMonster, double pNow, float pDeltaSeconds);
    uint64_t findTarget(const Monster& pMonster, const PlayerState& pState);
    bool moveTowards(Monster& pMonster, const PlayerState& pState, float pX, float pZ, float pStep);
};

}

#endif
