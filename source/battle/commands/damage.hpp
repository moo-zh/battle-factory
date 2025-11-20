/**
 * @file battle/commands/damage.hpp
 * @brief Damage calculation and application commands
 */

#pragma once

#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Calculate damage using simplified Gen III formula
 *
 * CONTRACT:
 * - Inputs: ctx.attacker stats, ctx.defender stats, ctx.move->power
 * - Outputs: Sets ctx.damage_dealt
 * - Does: Calculate damage (simplified for Pass 1)
 * - Does NOT: Apply the damage (that's ApplyDamage's job)
 *
 * SIMPLIFIED FORMULA (Pass 1):
 * - No stat stages
 * - No critical hits
 * - No type effectiveness
 * - No STAB
 * - No weather/ability/item modifiers
 * - No random variance
 *
 * Formula: damage = (22 * power * attack / defense) / 50 + 2
 * (This is the Gen III formula for level 50 with all modifiers = 1)
 */
inline void CalculateDamage(BattleContext& ctx) {
    if (ctx.move_failed)
        return;

    // Get power (use override if set, otherwise move's base power)
    int power = ctx.override_power > 0 ? ctx.override_power : ctx.move->power;

    // For now, we assume all moves are physical (Normal type is physical in Gen III)
    // TODO: Add physical/special split based on type when we add more move types
    int attack = ctx.attacker->attack;
    int defense = ctx.defender->defense;

    // Simplified Gen III damage formula (level 50)
    // damage = (((2 * Level / 5 + 2) * Power * A / D) / 50) + 2
    // For level 50: damage = ((22 * Power * A / D) / 50) + 2
    int damage = ((22 * power * attack / defense) / 50) + 2;

    // Minimum damage is 1 (unless immune, but we don't have types yet)
    if (damage < 1) {
        damage = 1;
    }

    ctx.damage_dealt = static_cast<uint16_t>(damage);
}

/**
 * @brief Apply calculated damage to defender
 *
 * CONTRACT:
 * - Inputs: ctx.damage_dealt, ctx.defender
 * - Outputs: Modifies ctx.defender->current_hp
 * - Does: Subtract damage from HP, clamp to 0
 * - Does NOT: Calculate damage, check for faint
 */
inline void ApplyDamage(BattleContext& ctx) {
    if (ctx.move_failed)
        return;

    // Subtract damage
    if (ctx.damage_dealt >= ctx.defender->current_hp) {
        ctx.defender->current_hp = 0;
    } else {
        ctx.defender->current_hp -= ctx.damage_dealt;
    }
}

}  // namespace commands
}  // namespace battle
