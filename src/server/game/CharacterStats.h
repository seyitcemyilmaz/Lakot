#ifndef LAKOT_SERVER_CHARACTERSTATS_H
#define LAKOT_SERVER_CHARACTERSTATS_H

#include <algorithm>
#include <cstdint>

namespace lakot
{

// A character's persistent numbers.
//
// Only BASE values live here. Everything derived (maximum health, attack
// power, defence) is computed from these plus equipment, never stored -
// otherwise every balance change would need a migration and a pass over every
// row, and a character's stored maximum could silently disagree with the
// formula that produced it.
//
// health/mana ARE stored, because current health is state, not a derivation -
// a player who logs out wounded logs back in wounded.
struct CharacterStats
{
    uint32_t level = 1;
    uint64_t experience = 0;

    int32_t health = 0;
    int32_t mana = 0;

    int32_t strength = 10;
    int32_t dexterity = 10;
    int32_t intelligence = 10;
    int32_t vitality = 10;

    uint64_t gold = 0;
};

// The formulas, kept together and free of any dependency on items or the
// database so they stay readable as pure balance decisions. Equipment bonuses
// are passed in by the caller (Character) rather than reached for here.
namespace StatFormula
{
    inline int32_t getMaxHealth(const CharacterStats& pStats, int32_t pEquipmentBonus)
    {
        return 50 + static_cast<int32_t>(pStats.level) * 10 + pStats.vitality * 5 + pEquipmentBonus;
    }

    inline int32_t getMaxMana(const CharacterStats& pStats, int32_t pEquipmentBonus)
    {
        return 20 + static_cast<int32_t>(pStats.level) * 5 + pStats.intelligence * 5 + pEquipmentBonus;
    }

    inline int32_t getAttack(const CharacterStats& pStats, int32_t pEquipmentBonus)
    {
        return 1 + pStats.strength * 2 + static_cast<int32_t>(pStats.level) + pEquipmentBonus;
    }

    inline int32_t getDefense(const CharacterStats& pStats, int32_t pEquipmentBonus)
    {
        return pStats.dexterity + static_cast<int32_t>(pStats.level) + pEquipmentBonus;
    }

    // Total experience needed to advance FROM pLevel to pLevel + 1.
    inline int32_t getDamage(int32_t pAttack, int32_t pDefense, float pRoll)
    {
        return std::max(1, static_cast<int32_t>(static_cast<float>(std::max(1, pAttack - pDefense / 2)) * pRoll));
    }

    inline uint64_t getExperienceForNextLevel(uint32_t pLevel)
    {
        return static_cast<uint64_t>(pLevel) * pLevel * 100ull;
    }

    // A brand new character starts at full health and mana. Used at creation
    // time; afterwards health is loaded from storage as-is.
    inline void initializeVitals(CharacterStats& pStats)
    {
        pStats.health = getMaxHealth(pStats, 0);
        pStats.mana = getMaxMana(pStats, 0);
    }

    // Keeps current health/mana inside their derived maxima - needed whenever
    // something that feeds a maximum changes (a level up, an item unequipped),
    // since the stored current value has no idea the ceiling moved.
    inline void clampVitals(CharacterStats& pStats, int32_t pMaxHealth, int32_t pMaxMana)
    {
        pStats.health = std::clamp(pStats.health, 0, pMaxHealth);
        pStats.mana = std::clamp(pStats.mana, 0, pMaxMana);
    }
}

}

#endif
