/**
 * @file test/test_effect_speed_down.cpp
 * @brief Tests for Effect_SpeedDown (String Shot)
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
TEST_CASE(Effect_SpeedDown_LowersSpeedStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -1,
                "String Shot should lower Speed by 1 stage");
}

TEST_CASE(Effect_SpeedDown_DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "String Shot should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "String Shot should not calculate damage");
}

TEST_CASE(Effect_SpeedDown_CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use String Shot 3 times: -1, -1, -1 = -3
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -3, "String Shot should stack to -3");
}

TEST_CASE(Effect_SpeedDown_MinimumStageMinus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -6, "Speed stage should not go below -6");
}

TEST_CASE(Effect_SpeedDown_CanLowerFromPositiveStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = +2;  // Boosted by Agility

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == +1, "String Shot should lower from +2 to +1");
}

TEST_CASE(Effect_SpeedDown_DoesNotModifyAttacker) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    int8_t original_stage = attacker.stat_stages[STAT_SPEED];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == original_stage,
                "String Shot should not affect attacker's stats");
}

TEST_CASE(Effect_SpeedDown_DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    // Only Speed should be lowered
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -1, "Speed should be -1");
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_SpeedDown_DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(!defender.is_fainted, "String Shot should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}
