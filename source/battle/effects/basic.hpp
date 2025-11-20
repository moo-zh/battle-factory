/**
 * @file battle/effects/basic.hpp
 * @brief Basic damage move effects
 */

#pragma once

#include "../commands/accuracy.hpp"
#include "../commands/damage.hpp"
#include "../commands/faint.hpp"
#include "../commands/status.hpp"
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

/**
 * @brief Effect: BURN_HIT - Damaging move with burn chance (e.g., Ember)
 *
 * This effect deals damage and has a chance to inflict Burn status.
 * It:
 * 1. Checks accuracy
 * 2. Calculates damage
 * 3. Applies damage
 * 4. Attempts to apply burn (uses move's effect_chance)
 * 5. Checks for faint
 *
 * The burn is checked AFTER damage but BEFORE faint check, so a Pokemon
 * that survives the hit can be burned, but a fainted Pokemon cannot.
 *
 * Example moves:
 * - Ember (40 power, 10% burn)
 * - Flamethrower (95 power, 10% burn)
 * - Fire Blast (120 power, 10% burn)
 * - Scald (80 power, 30% burn)
 */
inline void Effect_BurnHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);
    commands::TryApplyBurn(ctx, ctx.move->effect_chance);
    commands::CheckFaint(ctx);
}

/**
 * @brief Effect: PARALYZE - Status-only paralysis move (e.g., Thunder Wave)
 *
 * This effect applies paralysis status without dealing damage.
 * It:
 * 1. Checks accuracy
 * 2. Attempts to apply paralysis (100% chance if it hits)
 *
 * No damage is dealt, so there's no damage calculation, damage application,
 * or faint check. This is the first **status-only** effect.
 *
 * Example moves:
 * - Thunder Wave (0 power, 100 accuracy, Electric type)
 * - Stun Spore (0 power, 75 accuracy, Grass type)
 * - Glare (0 power, 75 accuracy, Normal type)
 */
inline void Effect_Paralyze(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::TryApplyParalysis(ctx, 100);  // 100% chance if move hits
    // No CheckFaint - status-only moves don't deal damage
}

}  // namespace effects
}  // namespace battle
