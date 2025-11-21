/**
 * @file battle/commands/drain.hpp
 * @brief HP drain/healing commands
 */

#pragma once

#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Apply drain healing to attacker based on damage dealt
 *
 * CONTRACT:
 * - Inputs: ctx.attacker, ctx.damage_dealt, drain_percent
 * - Outputs: Modifies ctx.attacker->current_hp, ctx.drain_received
 * - Does: Calculate drain amount, heal attacker, clamp HP to max_hp
 * - Does NOT: Check for faint (that's CheckFaint's job)
 *
 * DRAIN CALCULATION:
 * - Drain amount = damage_dealt / divisor
 * - 50% drain: damage_dealt / 2 (Giga Drain, Absorb, Mega Drain, Drain Punch)
 * - 75% drain: (damage_dealt * 3) / 4 (Dream Eater)
 * - Minimum drain = 1 (if any damage was dealt)
 * - No drain if move missed or dealt 0 damage
 *
 * EDGE CASES:
 * - Move failed: No drain
 * - Damage = 0: No drain
 * - Cannot overheal (HP clamped to max_hp)
 * - Already at max HP: Drain still calculated, but HP remains at max
 * - Liquid Ooze ability: Reverses drain to damage (future implementation)
 * - Big Root item: Increases drain by 30% (future implementation)
 *
 * Based on pokeemerald: src/battle_script_commands.c (Cmd_negativedamage)
 * gBattleMoveDamage = -(gHpDealt / 2); // negative damage = healing
 * if (gBattleMoveDamage == 0) gBattleMoveDamage = -1; // minimum 1
 *
 * @param ctx Battle context containing attacker, damage_dealt
 * @param drain_percent Percentage of damage to heal (typically 50)
 */
inline void ApplyDrain(BattleContext& ctx, uint8_t drain_percent) {
    // Guard: skip if move failed
    if (ctx.move_failed)
        return;

    // Guard: no drain if no damage was dealt
    if (ctx.damage_dealt == 0)
        return;

    // Calculate drain amount based on percentage
    uint16_t drain_amount;
    if (drain_percent == 50) {
        // 50% drain (1/2 of damage) - most common
        drain_amount = ctx.damage_dealt / 2;
    } else if (drain_percent == 75) {
        // 75% drain (3/4 of damage) - Dream Eater
        drain_amount = (ctx.damage_dealt * 3) / 4;
    } else {
        // Default to 50% if invalid percentage
        drain_amount = ctx.damage_dealt / 2;
    }

    // Minimum drain is 1 if any damage was dealt
    if (drain_amount == 0 && ctx.damage_dealt > 0) {
        drain_amount = 1;
    }

    // Apply drain to attacker (heal HP)
    uint16_t new_hp = ctx.attacker->current_hp + drain_amount;

    // Clamp to max HP (cannot overheal)
    if (new_hp > ctx.attacker->max_hp) {
        ctx.attacker->current_hp = ctx.attacker->max_hp;
    } else {
        ctx.attacker->current_hp = new_hp;
    }

    // Store drain amount for testing/display
    ctx.drain_received = drain_amount;

    // TODO (future): Check Liquid Ooze ability to reverse drain
    // if (HasAbility(ctx.defender, ABILITY_LIQUID_OOZE)) {
    //     // Reverse the healing - attacker takes damage instead
    //     if (drain_amount >= ctx.attacker->current_hp) {
    //         ctx.attacker->current_hp = 0;
    //     } else {
    //         ctx.attacker->current_hp -= drain_amount;
    //     }
    // }

    // TODO (future): Check Big Root item to increase drain
    // if (HasItem(ctx.attacker, ITEM_BIG_ROOT)) {
    //     uint16_t bonus = (drain_amount * 30) / 100; // +30%
    //     drain_amount += bonus;
    //     // Apply bonus healing (still clamped to max_hp)
    // }
}

}  // namespace commands
}  // namespace battle
