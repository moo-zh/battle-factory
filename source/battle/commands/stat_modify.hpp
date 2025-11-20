/**
 * @file battle/commands/stat_modify.hpp
 * @brief Stat stage modification commands
 */

#pragma once

#include "../../domain/stats.hpp"
#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Modify a Pokemon's stat stage
 *
 * CONTRACT:
 * - Inputs: ctx.defender, stat, change amount
 * - Outputs: Modifies ctx.defender->stat_stages[stat]
 * - Does: Clamps stat stage to -6..+6, checks if change occurred
 * - Does NOT: Deal damage, check accuracy (already done)
 *
 * STAT STAGE SYSTEM:
 * - Stat stages range from -6 to +6 (13 possible values, 0 is neutral)
 * - If a stat is already at min/max, the move fails silently (no change)
 * - Stage multipliers are applied during damage calculation:
 *   - If stage >= 0: multiplier = (2 + stage) / 2
 *   - If stage < 0:  multiplier = 2 / (2 - stage)
 *
 * EDGE CASES:
 * - Already at -6: Cannot go lower, no change
 * - Already at +6: Cannot go higher, no change
 * - Move already failed: Skip modification
 *
 * Based on pokeemerald: data/battle_scripts_1.s:516-554
 * BattleScript_EffectStatDown and statbuffchange commands
 *
 * @param ctx Battle context containing attacker, defender, move
 * @param stat Which stat to modify (STAT_ATK, STAT_DEF, etc.)
 * @param change How much to change the stage (negative to lower, positive to raise)
 */
inline void ModifyStatStage(BattleContext& ctx, domain::Stat stat, int8_t change) {
    // Guard: skip if move already failed
    if (ctx.move_failed)
        return;

    // Get current stage for this stat
    int8_t current_stage = ctx.defender->stat_stages[stat];

    // Calculate new stage (clamped to -6..+6)
    int16_t new_stage_unclamped = (int16_t)current_stage + (int16_t)change;
    int8_t new_stage = (int8_t)new_stage_unclamped;

    // Clamp to valid range
    if (new_stage < -6)
        new_stage = -6;
    if (new_stage > 6)
        new_stage = 6;

    // Check if change actually happened
    if (new_stage == current_stage) {
        // Stat won't go lower/higher
        // TODO (future): Set message flag for "won't go lower/higher"
        return;
    }

    // Apply the stat stage change
    ctx.defender->stat_stages[stat] = new_stage;

    // TODO (future): Set battle message
    // If change < 0: "[Pokemon]'s [Stat] fell!"
    // If change > 0: "[Pokemon]'s [Stat] rose!"
}

}  // namespace commands
}  // namespace battle
