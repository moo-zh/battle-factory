/**
 * @file battle/commands/recoil.hpp
 * @brief Recoil damage commands
 */

#pragma once

#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Apply recoil damage to attacker based on damage dealt
 *
 * CONTRACT:
 * - Inputs: ctx.attacker, ctx.damage_dealt, recoil_percent
 * - Outputs: Modifies ctx.attacker->current_hp, ctx.recoil_dealt
 * - Does: Calculate recoil damage, apply to attacker, clamp HP to 0
 * - Does NOT: Check for faint (that's CheckFaint's job)
 *
 * RECOIL CALCULATION:
 * - Recoil damage = damage_dealt / divisor
 * - 33% recoil: damage_dealt / 3 (Double-Edge, Brave Bird, Flare Blitz)
 * - 25% recoil: damage_dealt / 4 (Take Down, Submission)
 * - Minimum recoil = 1 (if any damage was dealt)
 * - No recoil if move missed or dealt 0 damage
 *
 * EDGE CASES:
 * - Move failed: No recoil
 * - Damage = 0: No recoil
 * - Recoil can reduce attacker HP to 0 (attacker can faint)
 * - Recoil HP cannot go negative (clamp to 0)
 * - Rock Head ability: Prevents recoil (future implementation)
 *
 * Based on pokeemerald: src/battle_script_commands.c
 * MOVE_EFFECT_RECOIL_25 and MOVE_EFFECT_RECOIL_33
 *
 * @param ctx Battle context containing attacker, damage_dealt
 * @param recoil_percent Percentage of damage as recoil (25 or 33)
 */
inline void ApplyRecoil(BattleContext& ctx, uint8_t recoil_percent) {
    // Guard: skip if move failed
    if (ctx.move_failed)
        return;

    // Guard: no recoil if no damage was dealt
    if (ctx.damage_dealt == 0)
        return;

    // Calculate recoil damage based on percentage
    uint16_t recoil_damage;
    if (recoil_percent == 33) {
        // 33% recoil (1/3 of damage)
        recoil_damage = ctx.damage_dealt / 3;
    } else if (recoil_percent == 25) {
        // 25% recoil (1/4 of damage)
        recoil_damage = ctx.damage_dealt / 4;
    } else {
        // Default to 33% if invalid percentage
        recoil_damage = ctx.damage_dealt / 3;
    }

    // Minimum recoil is 1 if any damage was dealt
    if (recoil_damage == 0 && ctx.damage_dealt > 0) {
        recoil_damage = 1;
    }

    // Apply recoil to attacker
    if (recoil_damage >= ctx.attacker->current_hp) {
        // Recoil kills attacker
        ctx.attacker->current_hp = 0;
    } else {
        // Subtract recoil from attacker HP
        ctx.attacker->current_hp -= recoil_damage;
    }

    // Store recoil amount for testing/display
    ctx.recoil_dealt = recoil_damage;

    // TODO (future): Check Rock Head ability to prevent recoil
    // if (HasAbility(ctx.attacker, ABILITY_ROCK_HEAD)) {
    //     ctx.attacker->current_hp += recoil_damage; // Restore HP
    //     ctx.recoil_dealt = 0;
    // }
}

}  // namespace commands
}  // namespace battle
