#ifndef LAKOT_STATSMESSAGE_H
#define LAKOT_STATSMESSAGE_H

#include <connection.pb.h>

#include "GameTypes.h"

namespace lakot::StatsMessage
{

inline CharacterStats read(const services::game::CharacterStats& pIncoming)
{
    CharacterStats tStats;
    tStats.level = pIncoming.level();
    tStats.experience = pIncoming.experience();
    tStats.experienceForNextLevel = pIncoming.experience_for_next_level();
    tStats.health = pIncoming.health();
    tStats.maxHealth = pIncoming.max_health();
    tStats.mana = pIncoming.mana();
    tStats.maxMana = pIncoming.max_mana();
    tStats.strength = pIncoming.strength();
    tStats.dexterity = pIncoming.dexterity();
    tStats.intelligence = pIncoming.intelligence();
    tStats.vitality = pIncoming.vitality();
    tStats.attack = pIncoming.attack();
    tStats.defense = pIncoming.defense();
    tStats.gold = pIncoming.gold();
    tStats.classId = pIncoming.class_id();
    tStats.pathId = pIncoming.path_id();
    tStats.statPoints = pIncoming.stat_points();
    return tStats;
}

}

#endif
