/**
 * @file battle/effects/basic.hpp
 * @brief Basic damage move effects
 */

#pragma once

#include "../../domain/stats.hpp"
#include "../commands/accuracy.hpp"
#include "../commands/damage.hpp"
#include "../commands/drain.hpp"
#include "../commands/faint.hpp"
#include "../commands/recoil.hpp"
#include "../commands/stat_modify.hpp"
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

/**
 * @brief Effect: ATTACK_DOWN - Lowers target's Attack by 1 stage (e.g., Growl)
 *
 * This effect lowers the target's Attack stat by 1 stage without dealing damage.
 * It:
 * 1. Checks accuracy
 * 2. Lowers Attack stat stage by 1
 *
 * This is the first **stat modification** effect, introducing the stat stage system.
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - If stage >= 0: multiplier = (2 + stage) / 2
 * - If stage < 0:  multiplier = 2 / (2 - stage)
 *
 * No damage is dealt, so there's no damage calculation, damage application,
 * or faint check.
 *
 * Example moves:
 * - Growl (0 power, 100 accuracy, Normal type)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:516-554
 */
inline void Effect_AttackDown(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::ModifyStatStage(ctx, domain::STAT_ATK, -1);  // Lower Attack by 1 stage
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: DEFENSE_DOWN - Lowers target's Defense by 1 stage (e.g., Tail Whip)
 *
 * This effect lowers the target's Defense stat by 1 stage without dealing damage.
 * It:
 * 1. Checks accuracy
 * 2. Lowers Defense stat stage by 1
 *
 * This mirrors Effect_AttackDown but targets Defense instead, demonstrating that
 * the stat stage system works for different stats.
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - If stage >= 0: multiplier = (2 + stage) / 2
 * - If stage < 0:  multiplier = 2 / (2 - stage)
 *
 * No damage is dealt, so there's no damage calculation, damage application,
 * or faint check.
 *
 * Example moves:
 * - Tail Whip (0 power, 100 accuracy, Normal type)
 * - Leer (0 power, 100 accuracy, Normal type)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:555-558
 */
inline void Effect_DefenseDown(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::ModifyStatStage(ctx, domain::STAT_DEF, -1);  // Lower Defense by 1 stage
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: SPEED_DOWN - Lowers target's Speed by 1 stage (e.g., String Shot)
 *
 * This effect lowers the target's Speed stat by 1 stage without dealing damage.
 * It:
 * 1. Checks accuracy
 * 2. Lowers Speed stat stage by 1
 *
 * This extends the stat stage system to **Speed** (the third stat validated),
 * demonstrating that the stat system works uniformly across Attack, Defense, and Speed.
 *
 * Stat stages range from -6 to +6, and apply multipliers:
 * - If stage >= 0: multiplier = (2 + stage) / 2
 * - If stage < 0:  multiplier = 2 / (2 - stage)
 * - Stage -1 = 0.67x Speed (~33% slower)
 *
 * Speed affects turn order in battle. Lower Speed = acts later in the turn.
 *
 * No damage is dealt, so there's no damage calculation, damage application,
 * or faint check.
 *
 * Example moves:
 * - String Shot (0 power, 95 accuracy, Bug type)
 * - Cotton Spore (0 power, 100 accuracy, Grass type, Speed -2)
 * - Scary Face (0 power, 100 accuracy, Normal type, Speed -2)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:524-526, 555-583
 */
inline void Effect_SpeedDown(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::ModifyStatStage(ctx, domain::STAT_SPEED, -1);  // Lower Speed by 1 stage
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: ATTACK_UP_2 - Raises user's Attack by 2 stages (e.g., Swords Dance)
 *
 * This effect raises the user's Attack stat by 2 stages without dealing damage.
 * It:
 * 1. Raises user's Attack stat stage by 2
 *
 * This is the first **self-targeting stat move**, introducing the concept that
 * stat changes can affect the attacker instead of the defender. This is a
 * fundamental setup move pattern in Pokemon.
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - Stage +2: multiplier = (2 + 2) / 2 = 2.0x (doubles damage output)
 *
 * No accuracy check (self-targeting moves cannot miss), no damage dealt,
 * no faint check.
 *
 * Example moves:
 * - Swords Dance (0 power, 0 accuracy, Normal type)
 * - Nasty Plot (Sp. Attack +2)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:719-721, 787-803
 */
inline void Effect_AttackUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss (accuracy = 0 in move data)
    commands::ModifyStatStage(ctx, domain::STAT_ATK, +2, /* affects_user= */ true);
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: DEFENSE_UP_2 - Raises user's Defense by 2 stages (e.g., Iron Defense)
 *
 * This effect raises the user's Defense stat by 2 stages without dealing damage.
 * It:
 * 1. Raises user's Defense stat stage by 2
 *
 * This is the **defensive counterpart to Swords Dance**, validating that the stat
 * stage system works correctly for defensive stats. This is a fundamental defensive
 * setup move pattern in Pokemon.
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - Stage +2: multiplier = (2 + 2) / 2 = 2.0x (doubles effective Defense)
 * - This reduces physical damage taken by approximately 50%
 *
 * No accuracy check (self-targeting moves cannot miss), no damage dealt,
 * no faint check.
 *
 * Example moves:
 * - Iron Defense (0 power, 0 accuracy, Steel type)
 * - Barrier (0 power, 0 accuracy, Psychic type)
 * - Acid Armor (0 power, 0 accuracy, Poison type)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:931-933, 787-803
 */
inline void Effect_DefenseUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss (accuracy = 0 in move data)
    commands::ModifyStatStage(ctx, domain::STAT_DEF, +2, /* affects_user= */ true);
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: RECOIL_HIT - Damaging move with recoil (e.g., Double-Edge)
 *
 * This effect deals damage to the target, then the attacker takes recoil damage.
 * It:
 * 1. Checks accuracy
 * 2. Calculates damage
 * 3. Applies damage to target
 * 4. Applies recoil damage to attacker (33% of damage dealt)
 * 5. Checks if target fainted
 * 6. Checks if attacker fainted from recoil
 *
 * This is the first move where the **attacker can damage itself** through
 * move side effects. This introduces recoil mechanics.
 *
 * Recoil calculation:
 * - Recoil = damage_dealt / 3 (33% of damage)
 * - Minimum recoil = 1 (if any damage was dealt)
 * - No recoil if move misses or deals 0 damage
 *
 * Example moves:
 * - Double-Edge (120 power, 33% recoil, Normal type)
 * - Brave Bird (120 power, 33% recoil, Flying type)
 * - Flare Blitz (120 power, 33% recoil, Fire type, can burn)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:896-900
 * MOVE_EFFECT_RECOIL_33 in src/battle_script_commands.c
 */
inline void Effect_RecoilHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);       // Damage to defender
    commands::ApplyRecoil(ctx, 33);   // Recoil to attacker (33%)
    commands::CheckFaint(ctx);        // Check if defender fainted
    commands::CheckFaint(ctx, true);  // Check if attacker fainted from recoil
}

/**
 * @brief Effect: DRAIN_HIT - Damaging move with HP drain (e.g., Giga Drain)
 *
 * This effect deals damage to the target, then the attacker heals for 50% of damage dealt.
 * It:
 * 1. Checks accuracy
 * 2. Calculates damage
 * 3. Applies damage to target
 * 4. Applies drain healing to attacker (50% of damage dealt)
 * 5. Checks if target fainted
 * 6. Checks if attacker fainted (rarely happens, but possible)
 *
 * This is the mirror of recoil mechanics - instead of the **attacker damaging itself**,
 * the attacker **heals itself** based on damage dealt. This introduces drain mechanics.
 *
 * Drain calculation:
 * - Drain = damage_dealt / 2 (50% of damage)
 * - Minimum drain = 1 (if any damage was dealt)
 * - No drain if move misses or deals 0 damage
 * - Cannot overheal (HP clamped to max_hp)
 *
 * Example moves:
 * - Absorb (20 power, 50% drain, Grass type)
 * - Mega Drain (40 power, 50% drain, Grass type)
 * - Giga Drain (60 power, 50% drain, Grass type)
 * - Drain Punch (60 power, 50% drain, Fighting type)
 * - Leech Life (20 power, 50% drain, Bug type)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:323-359 (BattleScript_EffectAbsorb)
 * Cmd_negativedamage in src/battle_script_commands.c:
 *   gBattleMoveDamage = -(gHpDealt / 2);  // negative = healing
 *   if (gBattleMoveDamage == 0) gBattleMoveDamage = -1;  // minimum 1
 */
inline void Effect_DrainHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);       // Damage to defender
    commands::ApplyDrain(ctx, 50);    // Drain to attacker (50%)
    commands::CheckFaint(ctx);        // Check if defender fainted
    commands::CheckFaint(ctx, true);  // Check if attacker fainted (rare)
}

/**
 * @brief Effect: SPEED_UP_2 - Raises user's Speed by 2 stages (e.g., Agility)
 *
 * This effect raises the user's Speed stat by 2 stages without dealing damage.
 * It:
 * 1. Raises user's Speed stat stage by 2
 *
 * This is the **Speed counterpart to Swords Dance** (Attack +2) and **completes the
 * Speed stat validation** (String Shot for Speed -1, Agility for Speed +2). This is
 * one of the most powerful setup moves in Pokemon - doubling effective Speed.
 *
 * Stat stages range from -6 to +6, and apply multipliers:
 * - Stage +2: multiplier = (2 + 2) / 2 = 2.0x (doubles effective Speed)
 *
 * Speed affects turn order in battle. Higher Speed = acts first in the turn.
 *
 * No accuracy check (self-targeting moves cannot miss), no damage dealt,
 * no faint check.
 *
 * Example moves:
 * - Agility (0 power, 0 accuracy, Psychic type)
 * - Rock Polish (0 power, 0 accuracy, Rock type)
 *
 * Based on pokeemerald: data/battle_scripts_1.s:935-937, 787-803
 */
inline void Effect_SpeedUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss (accuracy = 0 in move data)
    commands::ModifyStatStage(ctx, domain::STAT_SPEED, +2, /* affects_user= */ true);
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: SPECIAL_ATTACK_UP_2 - Raises user's Sp. Attack by 2 stages (e.g., Tail Glow)
 *
 * This effect raises the user's Special Attack stat by 2 stages without dealing damage.
 * It:
 * 1. Raises user's Sp. Attack stat stage by 2
 *
 * This is the **Special Attack counterpart to Swords Dance** (Attack +2), extending the
 * stat system to **special offensive stats**. This completes the offensive stat boost
 * validation (Attack +2 via Swords Dance, Sp. Attack +2 via Tail Glow).
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - Stage +2: multiplier = (2 + 2) / 2 = 2.0x (doubles special damage output)
 *
 * Sp. Attack affects the damage of special moves (Ember, Psychic, etc.).
 *
 * No accuracy check (self-targeting moves cannot miss), no damage dealt,
 * no faint check.
 *
 * Example moves:
 * - Tail Glow (0 power, 0 accuracy, Bug type) - pokeemerald ID 53
 * - Nasty Plot (0 power, 0 accuracy, Dark type) - Gen IV+
 *
 * Based on pokeemerald: include/constants/battle_move_effects.h:53, 787-803
 */
inline void Effect_SpecialAttackUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss (accuracy = 0 in move data)
    commands::ModifyStatStage(ctx, domain::STAT_SPATK, +2, /* affects_user= */ true);
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: SPECIAL_DEFENSE_DOWN_2 - Lowers target's Sp. Defense by 2 stages (e.g., Fake
 * Tears)
 *
 * This effect lowers the target's Special Defense stat by 2 stages without dealing damage.
 * It:
 * 1. Checks accuracy
 * 2. Lowers target's Sp. Defense stat stage by 2
 *
 * This extends the stat system to **special defensive stats**, introducing the concept of
 * **harshly lowering** a special defensive stat (-2 stages). This mirrors Tail Whip (Def -1)
 * but with a stronger reduction to special bulk.
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - Stage -2: multiplier = 2 / (2 - (-2)) = 2 / 4 = 0.5x (halves effective Sp. Defense)
 * - This approximately doubles special damage taken
 *
 * Sp. Defense affects damage reduction from special moves (Ember, Psychic, etc.).
 *
 * No damage dealt, no faint check.
 *
 * Example moves:
 * - Fake Tears (0 power, 100 accuracy, Dark type) - pokeemerald ID 62
 * - Metal Sound (0 power, 85 accuracy, Steel type)
 *
 * Based on pokeemerald: include/constants/battle_move_effects.h:62, 555-583
 */
inline void Effect_SpecialDefenseDown2(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);
    commands::ModifyStatStage(ctx, domain::STAT_SPDEF, -2);  // Lower Sp. Defense by 2 stages
    // No CheckFaint - status-only moves don't deal damage
}

/**
 * @brief Effect: SPECIAL_DEFENSE_UP_2 - Raises user's Sp. Defense by 2 stages (e.g., Amnesia)
 *
 * This effect raises the user's Special Defense stat by 2 stages without dealing damage.
 * It:
 * 1. Raises user's Sp. Defense stat stage by 2
 *
 * This is the **Special Defense counterpart to Iron Defense** (Defense +2), completing the
 * stat system validation for all **Gen III core stats** (HP, Attack, Defense, Speed,
 * Sp. Attack, Sp. Defense). This is one of the best special bulk setup moves.
 *
 * Stat stages range from -6 to +6, and apply multipliers during damage calculation:
 * - Stage +2: multiplier = (2 + 2) / 2 = 2.0x (doubles effective Sp. Defense)
 * - This reduces special damage taken by approximately 50%
 *
 * Sp. Defense affects damage reduction from special moves (Ember, Psychic, etc.).
 *
 * No accuracy check (self-targeting moves cannot miss), no damage dealt,
 * no faint check.
 *
 * Example moves:
 * - Amnesia (0 power, 0 accuracy, Psychic type) - pokeemerald ID 54
 *
 * Based on pokeemerald: include/constants/battle_move_effects.h:54, 787-803
 */
inline void Effect_SpecialDefenseUp2(BattleContext& ctx) {
    // No AccuracyCheck - self-targeting moves can't miss (accuracy = 0 in move data)
    commands::ModifyStatStage(ctx, domain::STAT_SPDEF, +2, /* affects_user= */ true);
    // No CheckFaint - status-only moves don't deal damage
}

}  // namespace effects
}  // namespace battle
