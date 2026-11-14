#ifndef LAKOT_COMBATRULES_H
#define LAKOT_COMBATRULES_H

#include <cstdint>

namespace lakot::CombatRules
{
    inline constexpr double kPlayerAttackInterval = 0.5;
    inline constexpr double kAttackIntervalTolerance = 0.1;

    inline constexpr float kPlayerAttackRange = 2.5f;
    inline constexpr float kAttackArcCos = 0.5f;
    inline constexpr float kBodyRadius = 0.5f;

    inline constexpr double kClientAttackMargin = 0.1;

    inline constexpr uint32_t kComboCount = 3;
    inline constexpr double kComboWindowSeconds = 1.0;
}

#endif
