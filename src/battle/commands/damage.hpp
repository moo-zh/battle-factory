/**
 * @file battle/commands/damage.hpp
 * @brief Damage calculation and application commands
 */

#pragma once

#include "../../domain/stats.hpp"
#include "../../domain/status.hpp"
#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Get a stat value with stat stage multiplier and status modifiers applied
 *
 * CONTRACT:
 * - Inputs: Pokemon with base stats, stat_stages, and status1
 * - Outputs: Returns modified stat value
 * - Does: Apply stage multiplier to base stat, then status modifiers
 *
 * STAT STAGE MULTIPLIERS:
 * - If stage >= 0: multiplier = (2 + stage) / 2
 * - If stage < 0:  multiplier = 2 / (2 - stage)
 *
 * Examples:
 * - Stage -6: 2/8 = 0.25x (25%)
 * - Stage -1: 2/3 = 0.67x (67%)
 * - Stage  0: 2/2 = 1.00x (100%, neutral)
 * - Stage +1: 3/2 = 1.50x (150%)
 * - Stage +6: 8/2 = 4.00x (400%)
 *
 * STATUS MODIFIERS (applied AFTER stat stages):
 * - Burn: Attack / 2 (50% reduction)
 * - Paralysis: Speed / 4 (handled in CalculateEffectiveSpeed, not here)
 *
 * Based on pokeemerald damage calculation with stat stages and status effects
 *
 * @param p Pokemon to get stat from
 * @param stat Which stat to retrieve (STAT_ATK, STAT_DEF, etc.)
 * @return Modified stat value with stage multiplier and status modifiers applied
 */
inline int GetModifiedStat(const state::Pokemon& p, domain::Stat stat) {
    int base_stat = 0;

    // Get base stat value
    switch (stat) {
        case domain::STAT_ATK:
            base_stat = p.attack;
            break;
        case domain::STAT_DEF:
            base_stat = p.defense;
            break;
        case domain::STAT_SPATK:
            base_stat = p.sp_attack;
            break;
        case domain::STAT_SPDEF:
            base_stat = p.sp_defense;
            break;
        case domain::STAT_SPEED:
            base_stat = p.speed;
            break;
        default:
            return base_stat;  // HP doesn't have stages
    }

    int8_t stage = p.stat_stages[stat];

    // Apply stage multiplier
    int modified_stat;
    if (stage >= 0) {
        // Positive or neutral: (2 + stage) / 2
        modified_stat = (base_stat * (2 + stage)) / 2;
    } else {
        // Negative: 2 / (2 - stage)
        modified_stat = (base_stat * 2) / (2 - stage);
    }

    // Apply status modifiers AFTER stat stages
    // Burn: Attack reduced by 50% (divide by 2)
    // Based on pokeemerald: if (status1 & STATUS1_BURN && stat == STAT_ATK)
    if (stat == domain::STAT_ATK && (p.status1 & domain::Status1::BURN)) {
        modified_stat /= 2;
    }

    // Future: Paralysis speed reduction is handled in CalculateEffectiveSpeed
    // (not here, because it's only for turn order, not damage calculation)

    return modified_stat;
}

/**
 * @brief Calculate damage using simplified Gen III formula
 *
 * CONTRACT:
 * - Inputs: ctx.attacker stats, ctx.defender stats, ctx.move->power
 * - Outputs: Sets ctx.damage_dealt
 * - Does: Calculate damage with stat stages applied
 * - Does NOT: Apply the damage (that's ApplyDamage's job)
 *
 * FORMULA (with stat stages):
 * - Stat stages applied to Attack and Defense
 * - No critical hits
 * - No type effectiveness
 * - No STAB
 * - No weather/ability/item modifiers
 * - No random variance
 *
 * Formula: damage = (22 * power * modified_attack / modified_defense) / 50 + 2
 * (This is the Gen III formula for level 50 with all modifiers = 1)
 *
 * Stat stages range from -6 to +6:
 * - If stage >= 0: multiplier = (2 + stage) / 2
 * - If stage < 0:  multiplier = 2 / (2 - stage)
 */
inline void CalculateDamage(BattleContext& ctx) {
    if (ctx.move_failed)
        return;

    // Get power (use override if set, otherwise move's base power)
    int power = ctx.override_power > 0 ? ctx.override_power : ctx.move->power;

    // For now, we assume all moves are physical (Normal type is physical in Gen III)
    // TODO: Add physical/special split based on type when we add more move types
    // Get modified stats with stat stages applied
    int attack = GetModifiedStat(*ctx.attacker, domain::STAT_ATK);
    int defense = GetModifiedStat(*ctx.defender, domain::STAT_DEF);

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
