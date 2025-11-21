/**
 * @file test/test_effect_defense_down.cpp
 * @brief Tests for Effect_DefenseDown (Tail Whip)
 */

#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "../../../source/domain/stats.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../source/battle/effects/basic.hpp"
TEST_CASE(Effect_DefenseDown_LowersDefenseStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == -1, "Tail Whip should lower Defense by 1 stage");
}

TEST_CASE(Effect_DefenseDown_DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Tail Whip should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Tail Whip should not calculate damage");
}

TEST_CASE(Effect_DefenseDown_CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Tail Whip 3 times
    Effect_DefenseDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_DefenseDown(ctx);
    ctx.move_failed = false;
    Effect_DefenseDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == -3, "Tail Whip should stack to -3");
}

TEST_CASE(Effect_DefenseDown_MinimumStageMinus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();
    defender.stat_stages[STAT_DEF] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == -6, "Defense stage should not go below -6");
    // TODO: Check for "won't go lower" message once messages are implemented
}

TEST_CASE(Effect_DefenseDown_CanLowerFromPositiveStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();
    defender.stat_stages[STAT_DEF] = 2;  // Boosted by Iron Defense

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 1, "Tail Whip should lower from +2 to +1");
}

TEST_CASE(Effect_DefenseDown_DoesNotModifyAttacker) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();

    int8_t original_stage = attacker.stat_stages[STAT_DEF];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == original_stage,
                "Tail Whip should not affect attacker's stats");
}

TEST_CASE(Effect_DefenseDown_DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    // Only Defense should be lowered
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == -1, "Defense should be -1");
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_DefenseDown_DoesNotCauseFaint) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailWhip();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseDown(ctx);

    TEST_ASSERT(!defender.is_fainted, "Tail Whip should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

// Integration test: Verify Defense stages affect damage calculation
TEST_CASE(StatStages_DefenseStageAffectsDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with -1 Defense
    defender2.stat_stages[STAT_DEF] = -1;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t increased_damage = ctx2.damage_dealt;

    // With -1 Defense, damage should be increased (defender has 2/3 effective defense)
    // Damage formula: damage = ... * Attack / Defense
    // The actual multiplier depends on integer division in the damage formula
    // For Bulbasaur (Defense 10): 10 * 2 / 3 = 6 (integer division)
    // So damage increase is: 10/6 ≈ 1.67x (not exactly 1.5x due to rounding)
    TEST_ASSERT(increased_damage > normal_damage,
                "Damage should be increased with -1 Defense stage");

    // Verify the damage increase is reasonable (should be more than 1.4x but less than 1.8x)
    // This accounts for integer division and rounding in both stat stages and damage formula
    uint16_t min_expected = (normal_damage * 14) / 10;  // 1.4x
    uint16_t max_expected = (normal_damage * 18) / 10;  // 1.8x
    TEST_ASSERT(increased_damage >= min_expected && increased_damage <= max_expected,
                "Increased damage should be roughly 1.5-1.7x of normal damage");
}
