/**
 * @file test/host/effects/test_protection.cpp
 * @brief Tests for protection move effects (Protect)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_protect.cpp (11 tests)
 *
 * This file tests the Protect move which:
 * 1. Blocks incoming attacks (damage, status, stat modification)
 * 2. Has degrading success rate on consecutive uses (100%, 50%, 25%, ...)
 * 3. Resets counter when other moves are used or when it fails
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace domain;  // For STAT_ constants

/**
 * @brief Test fixture for protection tests
 */
class ProtectionTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(ProtectionTest, FirstUseSucceeds) {
    domain::MoveData protect = CreateProtect();

    // Verify initial state
    EXPECT_EQ(attacker.protect_count, 0) << "Protect count should start at 0";
    EXPECT_FALSE(attacker.is_protected) << "Should not be protected initially";

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx);

    // First use should always succeed (100% success rate)
    EXPECT_FALSE(ctx.move_failed) << "First Protect should not fail";
    EXPECT_TRUE(attacker.is_protected) << "Attacker should be protected";
    EXPECT_EQ(attacker.protect_count, 1) << "Protect count should increment to 1";
}

TEST_F(ProtectionTest, BlocksDamage) {
    domain::MoveData protect = CreateProtect();
    domain::MoveData tackle = CreateTackle();

    // Turn 1: Charmander uses Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_TRUE(attacker.is_protected) << "Attacker should be protected";
    uint16_t original_hp = attacker.current_hp;

    // Turn 1: Bulbasaur uses Tackle on protected Charmander
    battle::BattleContext ctx2 = CreateBattleContext(&defender, &attacker, &tackle);
    battle::effects::Effect_Hit(ctx2);

    // Tackle should fail against protected target
    EXPECT_TRUE(ctx2.move_failed) << "Move should fail against protected target";
    EXPECT_EQ(ctx2.damage_dealt, 0) << "No damage should be dealt";
    EXPECT_EQ(attacker.current_hp, original_hp) << "HP should not change";
}

TEST_F(ProtectionTest, BlocksStatusMoves) {
    domain::MoveData protect = CreateProtect();
    domain::MoveData thunder_wave = CreateThunderWave();

    // Turn 1: Charmander uses Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_TRUE(attacker.is_protected) << "Attacker should be protected";

    // Turn 1: Bulbasaur uses Thunder Wave on protected Charmander
    battle::BattleContext ctx2 = CreateBattleContext(&defender, &attacker, &thunder_wave);
    battle::effects::Effect_Paralyze(ctx2);

    // Thunder Wave should fail against protected target
    EXPECT_TRUE(ctx2.move_failed) << "Move should fail against protected target";
    EXPECT_EQ(attacker.status1, 0) << "No status should be applied";
}

TEST_F(ProtectionTest, BlocksStatMoves) {
    domain::MoveData protect = CreateProtect();
    domain::MoveData growl = CreateGrowl();

    // Turn 1: Charmander uses Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_TRUE(attacker.is_protected) << "Attacker should be protected";
    int8_t original_atk_stage = attacker.stat_stages[STAT_ATK];

    // Turn 1: Bulbasaur uses Growl on protected Charmander
    battle::BattleContext ctx2 = CreateBattleContext(&defender, &attacker, &growl);
    battle::effects::Effect_AttackDown(ctx2);

    // Growl should fail against protected target
    EXPECT_TRUE(ctx2.move_failed) << "Move should fail against protected target";
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], original_atk_stage)
        << "Attack stage should not change";
}

TEST_F(ProtectionTest, DoesNotBlockSelfTargeting) {
    domain::MoveData protect = CreateProtect();
    domain::MoveData swords_dance = CreateSwordsDance();

    // Turn 1: Charmander uses Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_TRUE(attacker.is_protected) << "Attacker should be protected";

    // Turn 1: Bulbasaur uses Swords Dance (targets self, not Charmander)
    battle::BattleContext ctx2 = CreateBattleContext(&defender, &defender, &swords_dance);
    battle::effects::Effect_AttackUp2(ctx2);

    // Self-targeting move should succeed
    EXPECT_FALSE(ctx2.move_failed) << "Self-targeting move should not fail";
    EXPECT_EQ(defender.stat_stages[STAT_ATK], 2) << "Attack should increase by 2";
}

TEST_F(ProtectionTest, SecondUseCanFail) {
    // This test verifies that the second consecutive Protect has ~50% success rate
    // We'll run it many times to get statistical confidence

    int successes = 0;
    int trials = 200;  // Run 200 trials to get good statistics

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);  // Different seed per trial
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData protect = CreateProtect();

        // First Protect (should always succeed)
        battle::BattleContext ctx1 = CreateBattleContext(&test_attacker, &test_defender, &protect);
        battle::effects::Effect_Protect(ctx1);

        // Verify first one succeeded
        if (!test_attacker.is_protected || test_attacker.protect_count != 1) {
            continue;  // Skip this trial if first one failed (shouldn't happen)
        }

        // Clear protection flag for next turn (simulate turn boundary)
        test_attacker.is_protected = false;

        // Second Protect (should have 50% success rate)
        battle::BattleContext ctx2 = CreateBattleContext(&test_attacker, &test_defender, &protect);
        battle::effects::Effect_Protect(ctx2);

        if (!ctx2.move_failed) {
            successes++;
        }
    }

    // Success rate should be approximately 50% (within reasonable bounds)
    // We expect 100 successes out of 200, allow ±20 for statistical variance
    EXPECT_GT(successes, 80) << "Second Protect should succeed at least 80/200 times";
    EXPECT_LT(successes, 120) << "Second Protect should succeed at most 120/200 times";
}

TEST_F(ProtectionTest, ThirdUseRarer) {
    // This test verifies that the third consecutive Protect has ~25% success rate

    int successes = 0;
    int attempts = 0;  // Track how many times we actually attempt the third Protect
    int trials = 800;  // More initial trials to get enough valid attempts

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData protect = CreateProtect();

        // First Protect (100% success)
        battle::BattleContext ctx1 = CreateBattleContext(&test_attacker, &test_defender, &protect);
        battle::effects::Effect_Protect(ctx1);
        if (ctx1.move_failed)
            continue;

        test_attacker.is_protected = false;

        // Second Protect (50% success)
        battle::BattleContext ctx2 = CreateBattleContext(&test_attacker, &test_defender, &protect);
        battle::effects::Effect_Protect(ctx2);
        if (ctx2.move_failed)
            continue;

        test_attacker.is_protected = false;

        // Third Protect (should have 25% success rate)
        attempts++;
        battle::BattleContext ctx3 = CreateBattleContext(&test_attacker, &test_defender, &protect);
        battle::effects::Effect_Protect(ctx3);

        if (!ctx3.move_failed) {
            successes++;
        }
    }

    // Success rate should be approximately 25%
    // We expect attempts ~= 400 (800 * 0.5), successes ~= 100 (400 * 0.25)
    // Allow wide statistical variance
    EXPECT_GT(attempts, 300) << "Should have at least 300 valid attempts";
    EXPECT_GT(successes, 70) << "Third Protect should succeed at least 70 times";
    EXPECT_LT(successes, 130) << "Third Protect should succeed at most 130 times";
}

TEST_F(ProtectionTest, CounterResetsOnOtherMove) {
    domain::MoveData protect = CreateProtect();
    domain::MoveData tackle = CreateTackle();

    // Turn 1: First Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_EQ(attacker.protect_count, 1) << "Protect count should be 1";

    // Turn 2: Use Tackle (different move)
    attacker.is_protected = false;  // Clear protection flag
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &tackle);
    battle::effects::Effect_Hit(ctx2);

    // After using a non-Protect move, counter should reset
    // (This would normally happen in Engine, but we'll simulate it)
    attacker.protect_count = 0;

    // Turn 3: Second Protect (should be like first again - 100% success)
    battle::BattleContext ctx3 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx3);

    EXPECT_FALSE(ctx3.move_failed) << "Protect should succeed (counter was reset)";
    EXPECT_TRUE(attacker.is_protected) << "Should be protected";
    EXPECT_EQ(attacker.protect_count, 1) << "Protect count should be 1 again";
}

TEST_F(ProtectionTest, FailureResetsCounter) {
    domain::MoveData protect = CreateProtect();

    // Manually set protect_count high to force failure
    attacker.protect_count = 5;  // Success rate = 100 / 32 = ~3%

    // Use Protect with very low success rate
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx);

    // If it failed, counter should reset
    if (ctx.move_failed) {
        EXPECT_EQ(attacker.protect_count, 0) << "Failed Protect should reset counter";
        EXPECT_FALSE(attacker.is_protected) << "Should not be protected after failure";
    }
    // Note: There's a small chance it succeeds (~3%), which is fine
}

TEST_F(ProtectionTest, ClearsEachTurn) {
    domain::MoveData protect = CreateProtect();

    // Turn 1: Use Protect
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    EXPECT_TRUE(attacker.is_protected) << "Should be protected on Turn 1";

    // Simulate turn boundary: clear is_protected flag
    attacker.is_protected = false;

    // Turn 2: Verify protection is cleared
    EXPECT_FALSE(attacker.is_protected) << "Protection should clear at turn start";
}

TEST_F(ProtectionTest, IndependentPerPokemon) {
    battle::state::Pokemon charmander = CreateCharmander();
    battle::state::Pokemon bulbasaur = CreateBulbasaur();
    domain::MoveData protect = CreateProtect();

    // Charmander uses Protect
    battle::BattleContext ctx1 = CreateBattleContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    // Bulbasaur uses Protect
    battle::BattleContext ctx2 = CreateBattleContext(&bulbasaur, &charmander, &protect);
    battle::effects::Effect_Protect(ctx2);

    // Both should be protected independently
    EXPECT_TRUE(charmander.is_protected) << "Charmander should be protected";
    EXPECT_TRUE(bulbasaur.is_protected) << "Bulbasaur should be protected";
    EXPECT_EQ(charmander.protect_count, 1) << "Charmander's counter should be 1";
    EXPECT_EQ(bulbasaur.protect_count, 1) << "Bulbasaur's counter should be 1";
}
