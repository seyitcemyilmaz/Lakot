#include "VitalsRegeneration.h"

#include <algorithm>

using namespace lakot;

VitalsRegeneration::VitalsRegeneration(std::unordered_map<uint64_t, Character>& pCharacters, const CombatSystem& pCombat)
    : mCharacters(pCharacters)
    , mCombat(pCombat)
{
}

RegenerationResult VitalsRegeneration::update(double pNow)
{
    RegenerationResult tResult;

    if (pNow < mNextRegenAt)
    {
        return tResult;
    }

    mNextRegenAt = pNow + StatFormula::kRegenIntervalSeconds;

    for (auto& [tCharacterId, tCharacter] : mCharacters)
    {
        if (tCharacter.isDead())
        {
            continue;
        }

        bool tInCombat = mCombat.isInCombat(tCharacterId, pNow);
        const CharacterStats& tStats = tCharacter.getStats();
        int32_t tMaxHealth = tCharacter.getMaxHealth();
        int32_t tMaxMana = tCharacter.getMaxMana();

        if (tStats.health < tMaxHealth)
        {
            tCharacter.setHealth(tStats.health + StatFormula::getHealthRegen(tMaxHealth, tInCombat));
            tResult.healthChanged.push_back(tCharacterId);
        }

        if (tStats.mana < tMaxMana)
        {
            tCharacter.setMana(tStats.mana + StatFormula::getManaRegen(tMaxMana, tInCombat));
            tResult.manaChanged.push_back(tCharacterId);
        }
    }

    return tResult;
}
