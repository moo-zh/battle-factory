/**
 * @file test/test_effect_defense_up_2.cpp
 * @brief Tests for Effect_DefenseUp2 (Iron Defense)
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
TEST_CASE(Effect_DefenseUp2_RaisesDefenseStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == +2,
                "Iron Defense should raise user's Defense by 2 stages");
}

TEST_CASE(Effect_DefenseUp2_DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Iron Defense should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Iron Defense should not calculate damage");
}

TEST_CASE(Effect_DefenseUp2_CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Iron Defense 3 times: +2, +2, +2 = +6 (capped)
    Effect_DefenseUp2(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_DefenseUp2(ctx);
    ctx.move_failed = false;
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == +6, "Iron Defense should stack to +6 (cap)");
}

TEST_CASE(Effect_DefenseUp2_MaximumStagePlus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == +6, "Defense stage should not go above +6");
}

TEST_CASE(Effect_DefenseUp2_CapsAtPlus6FromPlus5) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == +6,
                "Iron Defense should cap at +6 (only +1 effective from +5)");
}

TEST_CASE(Effect_DefenseUp2_CanRaiseFromNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();
    attacker.stat_stages[STAT_DEF] = -3;  // Lowered by Tail Whip 3x

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == -1, "Iron Defense should raise from -3 to -1");
}

TEST_CASE(Effect_DefenseUp2_DoesNotModifyDefender) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    int8_t original_stage = defender.stat_stages[STAT_DEF];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == original_stage,
                "Iron Defense should not affect defender's stats");
}

TEST_CASE(Effect_DefenseUp2_DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    // Only Defense should be raised
    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == +2, "Defense should be +2");
    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_DefenseUp2_DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateIronDefense();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DefenseUp2(ctx);

    TEST_ASSERT(!defender.is_fainted, "Iron Defense should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

// Integration test: Verify +2 Defense reduces damage taken
TEST_CASE(StatStages_DefenseUp2ReducesDamageTaken) {
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto attacker = CreateCharmander();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with +2 Defense (2x Defense = ~0.5x damage)
    defender2.stat_stages[STAT_DEF] = +2;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t reduced_damage = ctx2.damage_dealt;

    // With +2 Defense, damage should be reduced
    // Stage +2 gives Defense multiplier (2+2)/2 = 2.0x → ~50% damage reduction
    TEST_ASSERT(reduced_damage < normal_damage, "Damage should be reduced with +2 Defense stage");

    // Expect reduced damage to be approximately 40-60% of normal (allowing for rounding)
    uint16_t min_expected = (normal_damage * 40) / 100;
    uint16_t max_expected = (normal_damage * 60) / 100;
    TEST_ASSERT(reduced_damage >= min_expected && reduced_damage <= max_expected,
                "Reduced damage should be roughly 40-60% of normal damage");
}
