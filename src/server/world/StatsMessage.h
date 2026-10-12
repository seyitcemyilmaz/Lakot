#ifndef LAKOT_SERVER_STATSMESSAGE_H
#define LAKOT_SERVER_STATSMESSAGE_H

#include <connection.pb.h>

#include "../game/Character.h"

namespace lakot::StatsMessage
{

inline void fill(const Character& pCharacter, services::game::CharacterStats* pOut)
{
    const CharacterStats& tStats = pCharacter.getStats();

    pOut->set_level(tStats.level);
    pOut->set_experience(tStats.experience);
    pOut->set_experience_for_next_level(StatFormula::getExperienceForNextLevel(tStats.level));

    pOut->set_health(tStats.health);
    pOut->set_max_health(pCharacter.getMaxHealth());
    pOut->set_mana(tStats.mana);
    pOut->set_max_mana(pCharacter.getMaxMana());

    pOut->set_strength(tStats.strength);
    pOut->set_dexterity(tStats.dexterity);
    pOut->set_intelligence(tStats.intelligence);
    pOut->set_vitality(tStats.vitality);

    pOut->set_attack(pCharacter.getAttack());
    pOut->set_defense(pCharacter.getDefense());

    pOut->set_gold(tStats.gold);

    pOut->set_class_id(tStats.classId);
    pOut->set_path_id(tStats.pathId);
    pOut->set_stat_points(tStats.statPoints);
}

}

#endif
