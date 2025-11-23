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
#include "../commands/weather.hpp"
#include "../context.hpp"
#include "../random.hpp"

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

/**
 * @brief Effect: PROTECT - Protection move with degrading success rate (e.g., Protect)
 *
 * This effect provides protection from most incoming attacks, with success rate degrading
 * on consecutive uses. It:
 * 1. Calculates success rate based on protect_count: 100% / (2^n)
 * 2. Performs RNG check against success rate
 * 3. If successful:
 *    - Sets attacker->is_protected = true
 *    - Increments protect_count
 * 4. If failed:
 *    - Sets move_failed = true
 *    - Resets protect_count = 0
 *
 * This is the first **protection mechanic**, introducing the concept of blocking
 * incoming attacks with a degrading success rate on consecutive uses.
 *
 * Success rate formula:
 * - First use: 100% (2^0 = 1)
 * - Second consecutive use: 50% (2^1 = 2)
 * - Third consecutive use: 25% (2^2 = 4)
 * - Fourth consecutive use: 12.5% (2^3 = 8)
 *
 * Key mechanics:
 * - Self-targeting (attacker protects themselves, cannot miss)
 * - No accuracy check needed
 * - Sets is_protected flag (checked by other moves)
 * - Counter increments on success, resets on failure
 * - Protection is volatile (cleared each turn by Engine)
 *
 * Example moves:
 * - Protect (0 power, 0 accuracy, Normal type, +4 priority)
 * - Detect (0 power, 0 accuracy, Fighting type, +4 priority)
 *
 * Based on pokeemerald:
 * - data/battle_scripts_1.s:BattleScript_EffectProtect
 * - src/battle_script_commands.c:Cmd_protectaffects
 * - gProtectStructs[battler].protected flag
 */
inline void Effect_Protect(BattleContext& ctx) {
    // Calculate success rate: 100 / (2^protect_count)
    // protect_count=0 → 100%, protect_count=1 → 50%, protect_count=2 → 25%, etc.
    uint8_t denominator = 1 << ctx.attacker->protect_count;  // 2^protect_count
    uint8_t success_rate = 100 / denominator;

    // RNG check: random(100) < success_rate
    uint8_t roll = random::Random(100);

    if (roll < success_rate) {
        // Success: Set protection and increment counter
        ctx.attacker->is_protected = true;
        ctx.attacker->protect_count++;
        ctx.move_failed = false;
    } else {
        // Failure: Reset counter and mark move as failed
        ctx.attacker->protect_count = 0;
        ctx.attacker->is_protected = false;
        ctx.move_failed = true;
    }
}

/**
 * @brief Effect: SOLAR_BEAM - Two-turn move (charges Turn 1, attacks Turn 2)
 *
 * This effect implements the two-turn charging mechanic. On the first turn, the Pokemon
 * charges energy (no damage). On the second turn, the move executes with full power.
 *
 * Turn 1 (Charging):
 * 1. Check if is_charging is false
 * 2. If false: Begin charging
 *    - Set is_charging = true
 *    - Set charging_move = Move::SolarBeam
 *    - No damage dealt
 *
 * Turn 2 (Attack):
 * 1. Check if is_charging is true
 * 2. If true: Execute attack
 *    - Clear is_charging = false
 *    - AccuracyCheck
 *    - CalculateDamage
 *    - ApplyDamage
 *    - CheckFaint
 *
 * This is the **first two-turn move** implementation, establishing the pattern for:
 * - Razor Wind, Sky Attack, Skull Bash (charge + attack)
 * - Fly, Dig, Bounce (charge + semi-invulnerable)
 *
 * Key mechanics:
 * - Turn 1: No damage, sets charging state
 * - Turn 2: Full damage calculation (120 power)
 * - Accuracy checked on Turn 2 only
 * - If move misses, charging is still consumed
 * - Future: Sunny Day skips charging (not implemented yet)
 *
 * Example moves:
 * - Solar Beam (120 power, 100 accuracy, Grass type)
 * - Razor Wind (80 power, 100 accuracy, Normal type, high crit)
 * - Sky Attack (140 power, 90 accuracy, Flying type, flinch chance)
 *
 * Based on pokeemerald:
 * - include/constants/battle_move_effects.h:155 (EFFECT_SOLAR_BEAM)
 * - data/battle_scripts_1.s:1903-1918 (BattleScript_EffectSolarBeam)
 * - data/battle_scripts_1.s:785-803 (Charging turn logic)
 * - STATUS2_MULTIPLETURNS flag (bit 12)
 */
inline void Effect_SolarBeam(BattleContext& ctx) {
    // Turn 1: Start charging
    if (!ctx.attacker->is_charging) {
        ctx.attacker->is_charging = true;
        ctx.attacker->charging_move = domain::Move::SolarBeam;
        ctx.move_failed = false;  // Move succeeded in starting
        // No damage dealt on charging turn
        return;
    }

    // Turn 2: Execute attack
    ctx.attacker->is_charging = false;  // Clear charging flag

    // Standard damage sequence
    commands::AccuracyCheck(ctx);
    if (ctx.move_failed)
        return;

    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);
    commands::CheckFaint(ctx);
}

/**
 * @brief Effect: SEMI_INVULNERABLE - Two-turn semi-invulnerable move (e.g., Fly)
 *
 * This effect implements the semi-invulnerable two-turn move mechanic. On Turn 1, the user
 * enters a semi-invulnerable state (airborne, underground, or underwater), consuming PP.
 * On Turn 2, the user attacks with normal damage calculation.
 *
 * **Key difference from Solar Beam:** While charging, the user is SEMI-INVULNERABLE and
 * cannot be targeted by most moves. Only specific moves can hit semi-invulnerable targets
 * (e.g., Gust/Thunder vs Fly, Earthquake vs Dig).
 *
 * This effect handles:
 * 1. Turn 1: Enter semi-invulnerable state
 *    - Set is_charging = true
 *    - Set is_semi_invulnerable = true
 *    - Set semi_invulnerable_type based on move (OnAir for Fly)
 *    - No damage dealt, return early
 * 2. Turn 2: Attack from semi-invulnerable state
 *    - Clear is_charging, is_semi_invulnerable, semi_invulnerable_type
 *    - Standard damage sequence (AccuracyCheck → CalculateDamage → ApplyDamage → CheckFaint)
 *
 * Semi-invulnerable types:
 * - OnAir: Fly, Bounce (can be hit by Gust, Thunder, Sky Uppercut, etc.)
 * - Underground: Dig (can be hit by Earthquake, Magnitude)
 * - Underwater: Dive (can be hit by Surf, Whirlpool)
 *
 * Example moves:
 * - Fly (70 power, 95 accuracy, Flying type, OnAir)
 * - Dig (80 power, 100 accuracy, Ground type, Underground)
 * - Dive (80 power, 100 accuracy, Water type, Underwater)
 * - Bounce (85 power, 85 accuracy, Flying type, OnAir)
 *
 * Based on pokeemerald:
 * - include/constants/battle_move_effects.h:159 (EFFECT_SEMI_INVULNERABLE)
 * - data/battle_scripts_1.s:1973-2003 (BattleScript_EffectSemiInvulnerable)
 * - src/battle_script_commands.c:9009-9026 (Cmd_setsemiinvulnerablebit)
 * - STATUS2_MULTIPLETURNS flag (bit 12, shared with Solar Beam)
 * - STATUS3_ON_AIR flag (bit 6)
 * - STATUS3_UNDERGROUND flag (bit 7)
 * - STATUS3_UNDERWATER (for Dive)
 *
 * Note: AccuracyCheck modifications for bypassing semi-invulnerable state are deferred
 * until we implement moves that can hit airborne/underground/underwater targets.
 */
inline void Effect_Fly(BattleContext& ctx) {
    // Turn 1: Fly up into the air (become semi-invulnerable)
    if (!ctx.attacker->is_charging) {
        ctx.attacker->is_charging = true;
        ctx.attacker->charging_move = domain::Move::Fly;
        ctx.attacker->is_semi_invulnerable = true;
        ctx.attacker->semi_invulnerable_type = state::SemiInvulnerableType::OnAir;
        ctx.move_failed = false;  // Move succeeded in starting
        // No damage dealt on fly-up turn
        return;
    }

    // Turn 2: Attack from the air (clear semi-invulnerable state)
    ctx.attacker->is_charging = false;           // Clear charging flag
    ctx.attacker->is_semi_invulnerable = false;  // Clear semi-invulnerable flag
    ctx.attacker->semi_invulnerable_type = state::SemiInvulnerableType::None;

    // Standard damage sequence
    commands::AccuracyCheck(ctx);
    if (ctx.move_failed)
        return;

    commands::CalculateDamage(ctx);
    commands::ApplyDamage(ctx);
    commands::CheckFaint(ctx);
}

/**
 * @brief Effect: SUBSTITUTE - Creates substitute at 25% HP cost (e.g., Substitute)
 *
 * This effect creates a decoy (substitute) that absorbs damage in place of the user.
 * The substitute is created by sacrificing 25% of max HP (rounded down, minimum 1).
 *
 * Mechanics:
 * 1. Check if user already has a substitute → if yes, fail
 * 2. Calculate HP cost: max_hp / 4 (minimum 1 HP)
 * 3. Check if user can afford cost (current_hp > cost) → if not, fail
 * 4. Deduct HP cost from user
 * 5. Set has_substitute = true
 * 6. Set substitute_hp = cost
 *
 * Failure conditions:
 * - Already has substitute (has_substitute == true)
 * - Insufficient HP (current_hp <= cost)
 *
 * Once created, the substitute absorbs all incoming damage until it breaks (substitute_hp reaches
 * 0).
 *
 * Example:
 * - User with 100 HP → cost = 25 HP, user left with 75 HP, substitute has 25 HP
 * - User with 35 HP → cost = 8 HP (35/4 rounds down), substitute has 8 HP
 * - User with 3 HP → cost = 1 HP (minimum), substitute has 1 HP
 * - User with 11 HP and max 45 HP → FAILS (cost is 11, need > 11)
 *
 * Based on pokeemerald:
 * - include/constants/battle_move_effects.h:95 (EFFECT_SUBSTITUTE)
 * - data/battle_scripts_1.s:1085-1109 (BattleScript_EffectSubstitute)
 * - src/battle_script_commands.c:7808-7833 (Cmd_setsubstitute)
 * - STATUS2_SUBSTITUTE flag (bit 24 of status2)
 * - gDisableStructs[battler].substituteHP
 *
 * Note: Damage absorption by substitute is NOT implemented yet.
 * This effect only handles substitute creation.
 */
inline void Effect_Substitute(BattleContext& ctx) {
    // Check if already has substitute
    if (ctx.attacker->has_substitute) {
        ctx.move_failed = true;
        return;
    }

    // Calculate HP cost (25% of max HP, minimum 1)
    uint16_t cost = ctx.attacker->max_hp / 4;
    if (cost == 0) {
        cost = 1;  // Minimum cost
    }

    // Check if can afford the cost (need STRICTLY GREATER than cost)
    if (ctx.attacker->current_hp <= cost) {
        ctx.move_failed = true;
        return;
    }

    // Create substitute
    ctx.attacker->current_hp -= cost;  // Deduct HP
    ctx.attacker->has_substitute = true;
    ctx.attacker->substitute_hp = cost;
    ctx.move_failed = false;  // Success
}

/**
 * @brief Effect: BATON_PASS - Transfers stat stages to target (e.g., Baton Pass)
 *
 * This effect implements the core mechanic of Baton Pass: transferring stat stage changes
 * from one Pokemon to another. In the full game, this occurs during a switch, but this
 * simplified implementation focuses on the stat transfer mechanism.
 *
 * Mechanics:
 * 1. Copy all 7 stat stages from attacker to defender
 *    - ATK, DEF, SPEED, SPATK, SPDEF, ACC, EVASION
 * 2. Defender's existing stages are overwritten (not added)
 * 3. Always succeeds (no failure conditions)
 *
 * What is transferred (in full implementation):
 * - Stat stages (all 7 stats)
 * - Substitute (if active)
 * - Aqua Ring, Ingrain (if active)
 * - Confusion (volatile status)
 * - Focus Energy (crit boost)
 *
 * What is NOT transferred:
 * - Primary status (burn, paralysis, freeze, sleep, poison)
 * - Semi-invulnerable state (Fly, Dig)
 * - Charging state (Solar Beam)
 *
 * Example usage (full game):
 * - Ninjask uses Speed Boost to get +6 Speed
 * - Ninjask uses Baton Pass
 * - Incoming sweeper (e.g., Blaziken) receives +6 Speed
 * - Sweeper can now outspeed and sweep the opponent's team
 *
 * Simplified implementation:
 * Since we don't have a party/switching system yet, this effect transfers stats
 * from attacker to defender. In the full implementation, defender would be the
 * incoming Pokemon selected by the player.
 *
 * Based on pokeemerald:
 * - include/constants/battle_move_effects.h:131 (EFFECT_BATON_PASS)
 * - data/battle_scripts_1.s:1694-1714 (BattleScript_EffectBatonPass)
 * - Stat transfer happens in switchindataupdate command
 */
inline void Effect_BatonPass(BattleContext& ctx) {
    // Transfer all stat stages from attacker to defender
    // In full implementation, defender would be the incoming Pokemon
    for (int i = 0; i < domain::NUM_BATTLE_STATS; i++) {
        ctx.defender->stat_stages[i] = ctx.attacker->stat_stages[i];
    }

    ctx.move_failed = false;  // Always succeeds
}

/**
 * @brief Effect: MULTI_HIT - Multi-hit move that strikes 2-5 times (e.g., Fury Attack)
 *
 * This effect performs a damaging attack 2-5 times in a single turn. The number of hits
 * is determined randomly at the start using pokeemerald's distribution algorithm.
 * It:
 * 1. Checks accuracy once (all subsequent hits land if first check passes)
 * 2. Determines hit count (2-5) using pokeemerald's weighted RNG
 * 3. For each hit:
 *    - Calculate damage (fresh crit roll each time)
 *    - Apply damage
 *    - Check if defender fainted → break early if so
 * 4. Check for faint after all hits
 *
 * This is the first **multi-hit mechanic**, introducing the concept of iterative damage
 * application with early termination on faint. Each hit is independent for damage and
 * crit calculation, but shares the same accuracy check.
 *
 * Hit count distribution (pokeemerald algorithm):
 * - 2 hits: 37.5% (3/8)
 * - 3 hits: 37.5% (3/8)
 * - 4 hits: 12.5% (1/8)
 * - 5 hits: 12.5% (1/8)
 *
 * Key mechanics:
 * - Single accuracy check at start
 * - Each hit has independent damage/crit rolls
 * - Early termination if defender faints mid-sequence
 * - Damage accumulates across all hits
 *
 * Example moves:
 * - Fury Attack (15 power, 85 accuracy, Normal type) - pokeemerald ID 29
 * - Double Slap (15 power, 85 accuracy, Normal type)
 * - Pin Missile (25 power, 95 accuracy, Bug type)
 * - Spike Cannon (20 power, 100 accuracy, Normal type)
 *
 * Based on pokeemerald:
 * - data/battle_scripts_1.s:BattleScript_EffectMultiHit
 * - src/battle_script_commands.c:Cmd_setmultihitcounter (hit count RNG)
 * - src/battle_script_commands.c:Cmd_decrementmultihit (loop control)
 */
inline void Effect_MultiHit(BattleContext& ctx) {
    commands::AccuracyCheck(ctx);

    // If move missed, no hits occur
    if (ctx.move_failed) {
        ctx.hit_count = 0;
        return;
    }

    // Determine hit count using pokeemerald's algorithm
    // First roll: 0-3
    uint8_t roll = random::Random(4);

    uint8_t hit_count;
    if (roll > 1) {
        // 2 or 3 on first roll → second roll for 2-5
        hit_count = random::Random(4) + 2;  // 2-5
    } else {
        // 0 or 1 on first roll → add 2 for 2-3
        hit_count = roll + 2;  // 2-3
    }

    ctx.hit_count = 0;          // Track actual hits landed
    uint16_t total_damage = 0;  // Accumulate damage across all hits

    // Execute each hit
    for (uint8_t i = 0; i < hit_count; i++) {
        // Calculate and apply damage for this hit
        commands::CalculateDamage(ctx);
        total_damage += ctx.damage_dealt;  // Accumulate damage
        commands::ApplyDamage(ctx);

        // Track successful hit
        ctx.hit_count++;

        // Early exit if defender fainted
        if (ctx.defender->current_hp == 0) {
            ctx.defender->is_fainted = true;
            break;
        }

        // Early exit if attacker fainted (shouldn't happen for Fury Attack, but safety check)
        if (ctx.attacker->current_hp == 0) {
            ctx.attacker->is_fainted = true;
            break;
        }
    }

    // Set total damage dealt across all hits
    ctx.damage_dealt = total_damage;

    // Final faint check
    commands::CheckFaint(ctx);
}

/**
 * @brief Effect: SANDSTORM - Summons a sandstorm for 5 turns
 *
 * Sets weather to Sandstorm, which:
 * - Lasts 5 turns by default (8 with Smooth Rock held item - not implemented)
 * - Deals 1/16 max HP damage to non-Rock/Ground/Steel types at end of turn
 * - Boosts Rock-type Special Defense by 50% (Gen IV+ - not implemented)
 *
 * This is a **weather-setting move**, the first to introduce global field state.
 * Weather affects end-of-turn processing and can modify damage calculations
 * for certain move types.
 *
 * Key mechanics:
 * - Replaces current weather (weather doesn't stack)
 * - Duration counter decrements each turn
 * - Weather ends when duration reaches 0
 * - Can fail if Sandstorm is already active (not implemented - allows refresh)
 *
 * Type immunities (for damage):
 * - Rock type: Immune
 * - Ground type: Immune
 * - Steel type: Immune
 * - Sand Veil ability: Takes damage but gains evasion boost (ability not implemented)
 *
 * Example move:
 * - Sandstorm (0 power, no accuracy check, Rock type, 10 PP) - pokeemerald ID 201
 *
 * Based on pokeemerald:
 * - data/battle_scripts_1.s:BattleScript_EffectSandstorm
 * - src/battle_weather.c:SetWeather
 * - src/battle_script_commands.c:Cmd_setweather
 */
inline void Effect_Sandstorm(BattleContext& ctx) {
    // Set sandstorm weather for 5 turns
    commands::SetWeather(ctx, domain::Weather::Sandstorm, 5);

    // TODO: Check if sandstorm is already active
    // TODO: Display message: "A sandstorm kicked up!"
}

/**
 * @brief Stealth Rock - Sets up entry hazard on opponent's side
 *
 * BEHAVIOR:
 * - Sets stealth_rock flag on defender's side (entry hazard)
 * - When a Pokemon switches in, takes damage based on type effectiveness vs Rock
 * - Damage = (max HP / 8) * type effectiveness
 * - Type effectiveness: 4x=50%, 2x=25%, 1x=12.5%, 0.5x=6.25%, 0.25x=3.125%
 * - Persists until battle ends (no way to remove in Gen III except Rapid Spin)
 * - Does not stack (setting again has no effect)
 *
 * MOVE PROPERTIES:
 * - Type: Rock
 * - Power: 0 (status move)
 * - Accuracy: Never misses (status move)
 * - PP: 20
 * - Priority: 0 (normal)
 * - Target: Opponent's side
 *
 * USAGE NOTES:
 * - Affects all Pokemon that switch in on the target side
 * - Flying types and Pokemon with Levitate still take damage (not affected by immunities)
 * - Damage is applied BEFORE any other switch-in effects (abilities, etc.)
 *
 * Based on pokeemerald:
 * - data/battle_scripts_1.s:BattleScript_EffectStealthRock
 * - src/battle_util.c:SetSideStatusBit (hazard application)
 * - src/battle_util.c:VARIOUS_TRY_ACTIVATE_STEALTH_ROCKS (switch-in damage)
 */
inline void Effect_StealthRock(BattleContext& ctx) {
    // Set stealth rock on defender's side
    if (!ctx.defender_side->stealth_rock) {
        ctx.defender_side->stealth_rock = true;
        // TODO: Display message: "Pointed stones float in the air around [side]!"
    } else {
        // Already set - move fails
        ctx.move_failed = true;
        // TODO: Display message: "But it failed!"
    }
}

}  // namespace effects
}  // namespace battle
