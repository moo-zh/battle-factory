/**
 * @file test/host/effects/test_volatile_state.cpp
 * @brief Tests for volatile state effects (Substitute, Baton Pass)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_substitute.cpp (14 tests)
 * - test/EZ80/archived/ti84ce/foundation/test_baton_pass.cpp (18 tests)
 *
 * This file tests volatile battle states that persist until the Pokemon switches out
 * or specific conditions are met (Substitute HP-based decoy, Baton Pass stat transfer).
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace domain;  // For STAT_ constants

// ============================================================================
// SUBSTITUTE TESTS
// ============================================================================

/**
 * @brief Test fixture for Substitute tests
 */
class SubstituteTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateBulbasaur();  // 45 HP
        defender = CreateCharmander();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(SubstituteTest, CreatesSuccessfully) {
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_TRUE(attacker.has_substitute) << "Substitute should be created";
    EXPECT_FALSE(ctx.move_failed) << "Move should succeed";
}

TEST_F(SubstituteTest, CostsCorrectHP) {
    uint16_t original_hp = attacker.current_hp;    // 45
    uint16_t expected_cost = attacker.max_hp / 4;  // 45 / 4 = 11
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_EQ(attacker.current_hp, original_hp - expected_cost)
        << "User HP should be reduced by 25% of max HP";
    EXPECT_EQ(attacker.current_hp, 34) << "45 - 11 = 34 HP remaining";
}

TEST_F(SubstituteTest, StoresCorrectSubHP) {
    uint16_t expected_sub_hp = attacker.max_hp / 4;  // 11
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_EQ(attacker.substitute_hp, expected_sub_hp) << "Substitute HP should be 25% of max HP";
    EXPECT_EQ(attacker.substitute_hp, 11) << "45 / 4 = 11 substitute HP";
}

TEST_F(SubstituteTest, RoundsDownCorrectly) {
    battle::state::Pokemon pikachu = CreatePikachu();  // 35 HP
    domain::MoveData move = CreateSubstitute();
    // 35 / 4 = 8.75, rounds down to 8

    battle::BattleContext ctx = CreateBattleContext(&pikachu, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_EQ(pikachu.substitute_hp, 8) << "35 / 4 = 8 (rounded down)";
    EXPECT_EQ(pikachu.current_hp, 27) << "35 - 8 = 27 HP remaining";
}

TEST_F(SubstituteTest, MinimumCost1HP) {
    // Create Pokemon with very low max HP (3)
    battle::state::Pokemon low_hp_mon = CreatePikachu();
    low_hp_mon.max_hp = 3;
    low_hp_mon.current_hp = 3;
    domain::MoveData move = CreateSubstitute();
    // 3 / 4 = 0, but minimum cost is 1

    battle::BattleContext ctx = CreateBattleContext(&low_hp_mon, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_EQ(low_hp_mon.substitute_hp, 1) << "Minimum substitute HP is 1";
    EXPECT_EQ(low_hp_mon.current_hp, 2) << "3 - 1 = 2 HP remaining";
}

TEST_F(SubstituteTest, FailsInsufficientHP) {
    domain::MoveData move = CreateSubstitute();
    // Cost is 11 HP (45 / 4)
    // Set HP to exactly 11 (at threshold, should fail)
    attacker.current_hp = 11;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_TRUE(ctx.move_failed) << "Should fail with insufficient HP";
    EXPECT_FALSE(attacker.has_substitute) << "No substitute should be created";
    EXPECT_EQ(attacker.current_hp, 11) << "HP should be unchanged";
}

TEST_F(SubstituteTest, FailsExactlyAtThreshold) {
    battle::state::Pokemon charmander = CreateCharmander();  // 39 HP
    uint16_t cost = charmander.max_hp / 4;                   // 39 / 4 = 9
    charmander.current_hp = cost;                            // Exactly at cost
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&charmander, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_TRUE(ctx.move_failed) << "Should fail when HP equals cost (need > cost)";
    EXPECT_FALSE(charmander.has_substitute) << "No substitute created";
    EXPECT_EQ(charmander.current_hp, cost) << "HP unchanged";
}

TEST_F(SubstituteTest, FailsAlreadyHasSubstitute) {
    domain::MoveData move = CreateSubstitute();

    // First substitute (should succeed)
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx1);
    EXPECT_TRUE(attacker.has_substitute) << "First substitute should succeed";

    // Second substitute attempt (should fail)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx2);
    EXPECT_TRUE(ctx2.move_failed) << "Should fail when already has substitute";
}

TEST_F(SubstituteTest, SucceedsWithMinimalHP) {
    uint16_t cost = attacker.max_hp / 4;  // 11
    // Set HP to cost + 1 (should succeed)
    attacker.current_hp = cost + 1;
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_FALSE(ctx.move_failed) << "Should succeed with HP > cost";
    EXPECT_TRUE(attacker.has_substitute) << "Substitute should be created";
    EXPECT_EQ(attacker.current_hp, 1) << "12 - 11 = 1 HP remaining";
}

TEST_F(SubstituteTest, CanRecreateAfterBreak) {
    domain::MoveData move = CreateSubstitute();

    // Create first substitute
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx1);
    EXPECT_TRUE(attacker.has_substitute) << "First substitute created";

    // Simulate substitute breaking (manual for now)
    attacker.has_substitute = false;
    attacker.substitute_hp = 0;

    // Create second substitute (should succeed)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx2);
    EXPECT_FALSE(ctx2.move_failed) << "Should succeed after substitute breaks";
    EXPECT_TRUE(attacker.has_substitute) << "Second substitute created";
}

TEST_F(SubstituteTest, OddMaxHP) {
    battle::state::Pokemon pikachu = CreatePikachu();  // 35 HP
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&pikachu, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    // 35 / 4 = 8.75 → 8
    EXPECT_EQ(pikachu.substitute_hp, 8) << "35 / 4 rounds down to 8";
    EXPECT_EQ(pikachu.current_hp, 27) << "35 - 8 = 27 HP remaining";
}

TEST_F(SubstituteTest, SetsAllFlags) {
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_TRUE(attacker.has_substitute) << "has_substitute flag should be set";
    EXPECT_GT(attacker.substitute_hp, 0) << "substitute_hp should be > 0";
    EXPECT_FALSE(ctx.move_failed) << "move_failed should be false";
}

TEST_F(SubstituteTest, NoChangesOnFailure) {
    attacker.current_hp = 10;  // Too low (need > 11)
    uint16_t original_hp = attacker.current_hp;
    domain::MoveData move = CreateSubstitute();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Substitute(ctx);

    EXPECT_TRUE(ctx.move_failed) << "Move should fail";
    EXPECT_FALSE(attacker.has_substitute) << "No substitute created";
    EXPECT_EQ(attacker.substitute_hp, 0) << "Substitute HP should be 0";
    EXPECT_EQ(attacker.current_hp, original_hp) << "HP should be unchanged";
}

// ============================================================================
// BATON PASS TESTS
// ============================================================================

/**
 * @brief Test fixture for Baton Pass tests
 */
class BatonPassTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateBulbasaur();
        defender = CreateCharmander();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(BatonPassTest, TransfersAllStats) {
    domain::MoveData move = CreateBatonPass();

    // Set up attacker with various stat stages
    attacker.stat_stages[STAT_ATK] = +2;
    attacker.stat_stages[STAT_DEF] = +1;
    attacker.stat_stages[STAT_SPEED] = -1;
    attacker.stat_stages[STAT_SPATK] = +3;
    attacker.stat_stages[STAT_SPDEF] = -2;
    attacker.stat_stages[STAT_ACC] = +1;
    attacker.stat_stages[STAT_EVASION] = 0;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], +2) << "ATK should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], +1) << "DEF should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -1) << "SPEED should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], +3) << "SPATK should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], -2) << "SPDEF should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_ACC], +1) << "ACC should be transferred";
    EXPECT_EQ(defender.stat_stages[STAT_EVASION], 0) << "EVASION should be transferred";
}

TEST_F(BatonPassTest, AlwaysSucceeds) {
    domain::MoveData move = CreateBatonPass();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_FALSE(ctx.move_failed) << "Baton Pass should always succeed";
}

TEST_F(BatonPassTest, TransfersMaxPositiveStages) {
    domain::MoveData move = CreateBatonPass();

    // Set all stats to +6 (max boost)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = +6;
    }

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        EXPECT_EQ(defender.stat_stages[i], +6) << "All stats should be +6";
    }
}

TEST_F(BatonPassTest, TransfersSinglePositiveStage) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_ATK] = +4;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], +4) << "ATK should be +4";
    // Other stats should be 0 (neutral)
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0) << "DEF should be 0";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0) << "SPEED should be 0";
}

TEST_F(BatonPassTest, TransfersNegativeStages) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_SPEED] = -3;
    attacker.stat_stages[STAT_ATK] = -1;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -3) << "SPEED should be -3";
    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1) << "ATK should be -1";
}

TEST_F(BatonPassTest, TransfersMaxNegativeStages) {
    domain::MoveData move = CreateBatonPass();

    // Set all stats to -6 (max drop)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = -6;
    }

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        EXPECT_EQ(defender.stat_stages[i], -6) << "All stats should be -6";
    }
}

TEST_F(BatonPassTest, OverwritesExistingStages) {
    domain::MoveData move = CreateBatonPass();

    // Defender starts with +2 ATK
    defender.stat_stages[STAT_ATK] = +2;

    // Attacker has -1 ATK
    attacker.stat_stages[STAT_ATK] = -1;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -1) << "ATK should be overwritten to -1 (not added)";
}

TEST_F(BatonPassTest, OverwritesAllExistingStages) {
    domain::MoveData move = CreateBatonPass();

    // Defender has various stat boosts
    defender.stat_stages[STAT_ATK] = +3;
    defender.stat_stages[STAT_DEF] = +2;
    defender.stat_stages[STAT_SPEED] = -1;

    // Attacker has different values
    attacker.stat_stages[STAT_ATK] = -2;
    attacker.stat_stages[STAT_DEF] = 0;
    attacker.stat_stages[STAT_SPEED] = +4;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], -2) << "ATK should be -2 (overwritten)";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0) << "DEF should be 0 (overwritten)";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], +4) << "SPEED should be +4 (overwritten)";
}

TEST_F(BatonPassTest, TransfersNeutralStages) {
    domain::MoveData move = CreateBatonPass();

    // Attacker has all neutral stages (0)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = 0;
    }

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        EXPECT_EQ(defender.stat_stages[i], 0) << "All stats should be 0 (neutral)";
    }
}

TEST_F(BatonPassTest, TransfersMixedStages) {
    domain::MoveData move = CreateBatonPass();

    // Set up complex mixed stat stages
    attacker.stat_stages[STAT_ATK] = +6;     // Max boost
    attacker.stat_stages[STAT_DEF] = +4;     // High boost
    attacker.stat_stages[STAT_SPEED] = -6;   // Max drop
    attacker.stat_stages[STAT_SPATK] = +2;   // Moderate boost
    attacker.stat_stages[STAT_SPDEF] = -3;   // Moderate drop
    attacker.stat_stages[STAT_ACC] = +1;     // Small boost
    attacker.stat_stages[STAT_EVASION] = 0;  // Neutral

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], +6) << "ATK should be +6";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], +4) << "DEF should be +4";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], -6) << "SPEED should be -6";
    EXPECT_EQ(defender.stat_stages[STAT_SPATK], +2) << "SPATK should be +2";
    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], -3) << "SPDEF should be -3";
    EXPECT_EQ(defender.stat_stages[STAT_ACC], +1) << "ACC should be +1";
    EXPECT_EQ(defender.stat_stages[STAT_EVASION], 0) << "EVASION should be 0";
}

TEST_F(BatonPassTest, TransfersAttack) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_ATK] = +5;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ATK], +5) << "ATK should be +5";
}

TEST_F(BatonPassTest, TransfersDefense) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_DEF] = +3;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_DEF], +3) << "DEF should be +3";
}

TEST_F(BatonPassTest, TransfersSpeed) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_SPEED] = +6;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPEED], +6) << "SPEED should be +6";
}

TEST_F(BatonPassTest, TransfersSpecialAttack) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_SPATK] = +4;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPATK], +4) << "SPATK should be +4";
}

TEST_F(BatonPassTest, TransfersSpecialDefense) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_SPDEF] = +2;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_SPDEF], +2) << "SPDEF should be +2";
}

TEST_F(BatonPassTest, TransfersAccuracy) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_ACC] = -2;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_ACC], -2) << "ACC should be -2";
}

TEST_F(BatonPassTest, TransfersEvasion) {
    domain::MoveData move = CreateBatonPass();

    attacker.stat_stages[STAT_EVASION] = +3;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BatonPass(ctx);

    EXPECT_EQ(defender.stat_stages[STAT_EVASION], +3) << "EVASION should be +3";
}

TEST_F(BatonPassTest, CanChainMultipleTimes) {
    battle::state::Pokemon pokemon1 = CreateBulbasaur();
    battle::state::Pokemon pokemon2 = CreateCharmander();
    battle::state::Pokemon pokemon3 = CreatePikachu();
    domain::MoveData move = CreateBatonPass();

    // Pokemon 1 has +3 ATK
    pokemon1.stat_stages[STAT_ATK] = +3;

    // Pass from Pokemon 1 to Pokemon 2
    battle::BattleContext ctx1 = CreateBattleContext(&pokemon1, &pokemon2, &move);
    battle::effects::Effect_BatonPass(ctx1);
    EXPECT_EQ(pokemon2.stat_stages[STAT_ATK], +3) << "Pokemon 2 should have +3 ATK";

    // Pass from Pokemon 2 to Pokemon 3
    battle::BattleContext ctx2 = CreateBattleContext(&pokemon2, &pokemon3, &move);
    battle::effects::Effect_BatonPass(ctx2);
    EXPECT_EQ(pokemon3.stat_stages[STAT_ATK], +3) << "Pokemon 3 should have +3 ATK";
}
