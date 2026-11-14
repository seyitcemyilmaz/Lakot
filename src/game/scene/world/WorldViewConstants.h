#ifndef LAKOT_WORLD_VIEW_CONSTANTS_H
#define LAKOT_WORLD_VIEW_CONSTANTS_H

#include <glm/glm.hpp>

#include "CombatRules.h"
#include "MovementRules.h"

namespace lakot::WorldView
{

    inline constexpr glm::vec3 kPlayerBoxHalfExtents(0.5f, MovementRules::kBodyHalfHeight, 0.5f);

    inline constexpr double kStateSendInterval = 0.1;

    inline constexpr double kMinRemoteInterpolationSeconds = 0.1;

    inline constexpr double kMaxRemoteInterpolationSeconds = 0.5;

    inline constexpr double kAttackIntervalSeconds = CombatRules::kPlayerAttackInterval + CombatRules::kClientAttackMargin;

    inline constexpr double kAttackSegmentSeconds = kAttackIntervalSeconds * 1.1;

    inline constexpr double kBasicHitStopSeconds = 0.05;

    inline constexpr float kBasicHitTrauma = 0.35f;

    inline constexpr float kHitConeRange = 3.5f;

    inline constexpr float kHitConeCosine = 0.5f;

    inline constexpr float kViewDistance = 360.0f;

    inline constexpr double kSpeechBubbleVisibleSeconds = 5.0;

    inline const glm::vec3 kLightDirection = glm::normalize(glm::vec3(0.45f, -0.8f, -0.35f));
}

#endif
