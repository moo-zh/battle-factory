/**
 * @file battle/effects/basic.hpp
 * @brief Basic damage move effects
 */

#pragma once

#include "../commands/accuracy.hpp"
#include "../commands/damage.hpp"
#include "../commands/faint.hpp"
#include "../context.hpp"

namespace battle {
namespace effects {

/**
 * @brief Effect: HIT - Basic damaging move (e.g., Tackle)
 *
 * This is the simplest damage effect. It:
 * 1. Checks accuracy
 * 2. Calculates damage
 * 3. Applies damage
 * 4. Checks for faint
 *
 * No secondary effects.
 *
 * Example moves: Tackle, Pound, Scratch
 */
inline void Effect_Hit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);
    commands::CheckFaint(ctx);
}

}  // namespace effects
}  // namespace battle
