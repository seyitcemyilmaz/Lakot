#ifndef LAKOT_ITEMRULES_H
#define LAKOT_ITEMRULES_H

namespace lakot::ItemRules
{
    inline constexpr float kPickupRange = 3.0f;
    inline constexpr float kClientPickupMargin = 0.2f;
    inline constexpr double kPickupCooldownSeconds = 0.1;

    inline constexpr float kShopRange = 5.0f;
    inline constexpr float kClientShopCloseMargin = 1.0f;
    inline constexpr double kShopActionCooldownSeconds = 0.2;
}

#endif
