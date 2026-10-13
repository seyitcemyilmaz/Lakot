#ifndef LAKOT_SERVER_VITALSREGENERATION_H
#define LAKOT_SERVER_VITALSREGENERATION_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "CombatSystem.h"

#include "../game/Character.h"

namespace lakot
{

struct RegenerationResult
{
    std::vector<uint64_t> healthChanged;
    std::vector<uint64_t> manaChanged;
};

class VitalsRegeneration
{
public:
    VitalsRegeneration(std::unordered_map<uint64_t, Character>& pCharacters, const CombatSystem& pCombat);

    RegenerationResult update(double pNow);

private:
    std::unordered_map<uint64_t, Character>& mCharacters;
    const CombatSystem& mCombat;
    double mNextRegenAt{0.0};
};

}

#endif
