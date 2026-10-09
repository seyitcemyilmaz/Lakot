#ifndef LAKOT_MOVEMENTRULES_H
#define LAKOT_MOVEMENTRULES_H

namespace lakot::MovementRules
{
    // Ground speed in world units per second. The client moves at it; the
    // server refuses anything faster.
    inline constexpr float kWalkSpeed = 5.0f;

    // Positions on the wire are body centres, this far above the ground.
    inline constexpr float kBodyHalfHeight = 1.0f;
}

#endif
