/**
 * @file test/unit/effects/test_special_attack_up_2.cpp
 * @brief Tests for Effect_SpecialAttackUp2 (Tail Glow)
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
TEST_CASE(Effect_SpecialAttackUp2_RaisesSpecialAttackStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +2,
                "Tail Glow should raise user's Sp. Attack by 2 stages");
}

TEST_CASE(Effect_SpecialAttackUp2_DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Tail Glow should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Tail Glow should not calculate damage");
}

TEST_CASE(Effect_SpecialAttackUp2_CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Tail Glow 3 times: +2, +2, +2 = +6 (capped)
    Effect_SpecialAttackUp2(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_SpecialAttackUp2(ctx);
    ctx.move_failed = false;
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +6, "Tail Glow should stack to +6 (cap)");
}

TEST_CASE(Effect_SpecialAttackUp2_MaximumStagePlus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();
    attacker.stat_stages[STAT_SPATK] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +6, "Sp. Attack stage should not go above +6");
}

TEST_CASE(Effect_SpecialAttackUp2_CapsAtPlus6FromPlus5) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();
    attacker.stat_stages[STAT_SPATK] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +6,
                "Tail Glow should cap at +6 (only +1 effective from +5)");
}

TEST_CASE(Effect_SpecialAttackUp2_CanRaiseFromNegativeStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();
    attacker.stat_stages[STAT_SPATK] = -2;  // Lowered by some move

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == 0, "Tail Glow should raise from -2 to 0");
}

TEST_CASE(Effect_SpecialAttackUp2_DoesNotModifyDefender) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();

    int8_t original_stage = defender.stat_stages[STAT_SPATK];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == original_stage,
                "Tail Glow should not affect defender's stats");
}

TEST_CASE(Effect_SpecialAttackUp2_DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    // Only Sp. Attack should be raised
    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +2, "Sp. Attack should be +2");
    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_SpecialAttackUp2_DoesNotCauseFaint) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    TEST_ASSERT(!defender.is_fainted, "Tail Glow should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

TEST_CASE(Effect_SpecialAttackUp2_NoAccuracyCheck) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTailGlow();
    move.accuracy = 0;  // Self-targeting moves have accuracy = 0

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialAttackUp2(ctx);

    // Move should never miss
    TEST_ASSERT(!ctx.move_failed, "Tail Glow should never miss (self-targeting)");
    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == +2,
                "Sp. Attack should be raised even with accuracy=0");
}
