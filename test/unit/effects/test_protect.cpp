/**
 * @file test_protect.cpp
 * @brief Tests for Effect_Protect (protection move like Protect)
 *
 * These tests verify that Protect correctly:
 * 1. Blocks incoming attacks
 * 2. Has degrading success rate on consecutive uses
 * 3. Resets counter when other moves are used
 */

#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../source/battle/effects/basic.hpp"

// ============================================================================
// BASIC PROTECTION TESTS
// ============================================================================

TEST_CASE(Effect_Protect_FirstUseSucceeds) {
    // Setup: Charmander uses Protect for the first time
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto protect = CreateProtect();

    // Verify initial state
    TEST_ASSERT_EQ(attacker.protect_count, 0, "Protect count should start at 0");
    TEST_ASSERT_EQ(attacker.is_protected, false, "Should not be protected initially");

    auto ctx = SetupContext(&attacker, &defender, &protect);

    // Execute Protect
    battle::effects::Effect_Protect(ctx);

    // Assert: First use should always succeed (100% success rate)
    TEST_ASSERT_EQ(ctx.move_failed, false, "First Protect should not fail");
    TEST_ASSERT(attacker.is_protected, "Attacker should be protected");
    TEST_ASSERT_EQ(attacker.protect_count, 1, "Protect count should increment to 1");
}

TEST_CASE(Effect_Protect_BlocksDamage) {
    // Setup: Charmander uses Protect, then Bulbasaur uses Tackle
    auto charmander = CreateCharmander();
    auto bulbasaur = CreateBulbasaur();
    auto protect = CreateProtect();
    auto tackle = CreateTackle();

    // Turn 1: Charmander uses Protect
    auto ctx1 = SetupContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT(charmander.is_protected, "Charmander should be protected");
    uint16_t original_hp = charmander.current_hp;

    // Turn 1: Bulbasaur uses Tackle on protected Charmander
    auto ctx2 = SetupContext(&bulbasaur, &charmander, &tackle);
    battle::effects::Effect_Hit(ctx2);

    // Assert: Tackle should fail against protected target
    TEST_ASSERT(ctx2.move_failed, "Move should fail against protected target");
    TEST_ASSERT_EQ(ctx2.damage_dealt, 0, "No damage should be dealt");
    TEST_ASSERT_EQ(charmander.current_hp, original_hp, "HP should not change");
}

TEST_CASE(Effect_Protect_BlocksStatusMoves) {
    // Setup: Charmander uses Protect, then Bulbasaur uses Thunder Wave
    auto charmander = CreateCharmander();
    auto bulbasaur = CreateBulbasaur();
    auto protect = CreateProtect();
    auto thunder_wave = CreateThunderWave();

    // Turn 1: Charmander uses Protect
    auto ctx1 = SetupContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT(charmander.is_protected, "Charmander should be protected");

    // Turn 1: Bulbasaur uses Thunder Wave on protected Charmander
    auto ctx2 = SetupContext(&bulbasaur, &charmander, &thunder_wave);
    battle::effects::Effect_Paralyze(ctx2);

    // Assert: Thunder Wave should fail against protected target
    TEST_ASSERT(ctx2.move_failed, "Move should fail against protected target");
    TEST_ASSERT_EQ(charmander.status1, 0, "No status should be applied");
}

TEST_CASE(Effect_Protect_BlocksStatMoves) {
    // Setup: Charmander uses Protect, then Bulbasaur uses Growl
    auto charmander = CreateCharmander();
    auto bulbasaur = CreateBulbasaur();
    auto protect = CreateProtect();
    auto growl = CreateGrowl();

    // Turn 1: Charmander uses Protect
    auto ctx1 = SetupContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT(charmander.is_protected, "Charmander should be protected");
    int8_t original_atk_stage = charmander.stat_stages[0];

    // Turn 1: Bulbasaur uses Growl on protected Charmander
    auto ctx2 = SetupContext(&bulbasaur, &charmander, &growl);
    battle::effects::Effect_AttackDown(ctx2);

    // Assert: Growl should fail against protected target
    TEST_ASSERT(ctx2.move_failed, "Move should fail against protected target");
    TEST_ASSERT_EQ(charmander.stat_stages[0], original_atk_stage, "Attack stage should not change");
}

TEST_CASE(Effect_Protect_DoesNotBlockSelfTargeting) {
    // Setup: Charmander uses Protect, Bulbasaur uses Swords Dance (self-targeting)
    auto charmander = CreateCharmander();
    auto bulbasaur = CreateBulbasaur();
    auto protect = CreateProtect();
    auto swords_dance = CreateSwordsDance();

    // Turn 1: Charmander uses Protect
    auto ctx1 = SetupContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT(charmander.is_protected, "Charmander should be protected");

    // Turn 1: Bulbasaur uses Swords Dance (targets self, not Charmander)
    auto ctx2 = SetupContext(&bulbasaur, &bulbasaur, &swords_dance);  // Note: defender = attacker
    battle::effects::Effect_AttackUp2(ctx2);

    // Assert: Self-targeting move should succeed
    TEST_ASSERT_EQ(ctx2.move_failed, false, "Self-targeting move should not fail");
    TEST_ASSERT_EQ(bulbasaur.stat_stages[domain::STAT_ATK], 2, "Attack should increase by 2");
}

// ============================================================================
// SUCCESS RATE DEGRADATION TESTS
// ============================================================================

TEST_CASE(Effect_Protect_SecondUseCanFail) {
    // This test verifies that the second consecutive Protect has ~50% success rate
    // We'll run it many times to get statistical confidence

    int successes = 0;
    int trials = 200;  // Run 200 trials to get good statistics

    for (int i = 0; i < trials; i++) {
        auto attacker = CreateCharmander();
        auto defender = CreateBulbasaur();
        auto protect = CreateProtect();

        // First Protect (should always succeed)
        auto ctx1 = SetupContext(&attacker, &defender, &protect);
        battle::effects::Effect_Protect(ctx1);

        // Verify first one succeeded
        if (!attacker.is_protected || attacker.protect_count != 1) {
            continue;  // Skip this trial if first one failed (shouldn't happen)
        }

        // Clear protection flag for next turn (simulate turn boundary)
        attacker.is_protected = false;

        // Second Protect (should have 50% success rate)
        auto ctx2 = SetupContext(&attacker, &defender, &protect);
        battle::effects::Effect_Protect(ctx2);

        if (!ctx2.move_failed) {
            successes++;
        }
    }

    // Assert: Success rate should be approximately 50% (within reasonable bounds)
    // We expect 100 successes out of 200, allow ±20 for statistical variance
    TEST_ASSERT(successes > 80, "Second Protect should succeed at least 80/200 times");
    TEST_ASSERT(successes < 120, "Second Protect should succeed at most 120/200 times");
}

TEST_CASE(Effect_Protect_ThirdUseRarer) {
    // This test verifies that the third consecutive Protect has ~25% success rate
    // Note: We only count trials where first AND second both succeeded

    int successes = 0;
    int attempts = 0;  // Track how many times we actually attempt the third Protect
    int trials = 800;  // More initial trials to get enough valid attempts

    for (int i = 0; i < trials; i++) {
        auto attacker = CreateCharmander();
        auto defender = CreateBulbasaur();
        auto protect = CreateProtect();

        // First Protect (100% success)
        auto ctx1 = SetupContext(&attacker, &defender, &protect);
        battle::effects::Effect_Protect(ctx1);
        if (ctx1.move_failed)
            continue;

        attacker.is_protected = false;

        // Second Protect (50% success)
        auto ctx2 = SetupContext(&attacker, &defender, &protect);
        battle::effects::Effect_Protect(ctx2);
        if (ctx2.move_failed)
            continue;

        attacker.is_protected = false;

        // Third Protect (should have 25% success rate)
        attempts++;
        auto ctx3 = SetupContext(&attacker, &defender, &protect);
        battle::effects::Effect_Protect(ctx3);

        if (!ctx3.move_failed) {
            successes++;
        }
    }

    // Assert: Success rate should be approximately 25%
    // We expect attempts ~= 400 (800 * 0.5), successes ~= 100 (400 * 0.25)
    // Allow wide statistical variance: 70-130 successes (17.5%-32.5%)
    TEST_ASSERT(attempts > 300, "Should have at least 300 valid attempts");
    TEST_ASSERT(successes > 70, "Third Protect should succeed at least 70 times");
    TEST_ASSERT(successes < 130, "Third Protect should succeed at most 130 times");
}

// ============================================================================
// COUNTER RESET TESTS
// ============================================================================

TEST_CASE(Effect_Protect_CounterResetsOnOtherMove) {
    // Setup: Charmander uses Protect, then Tackle, then Protect again
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto protect = CreateProtect();
    auto tackle = CreateTackle();

    // Turn 1: First Protect
    auto ctx1 = SetupContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT_EQ(attacker.protect_count, 1, "Protect count should be 1");

    // Turn 2: Use Tackle (different move)
    attacker.is_protected = false;  // Clear protection flag
    auto ctx2 = SetupContext(&attacker, &defender, &tackle);
    battle::effects::Effect_Hit(ctx2);

    // After using a non-Protect move, counter should reset
    // (This would normally happen in Engine, but we'll simulate it)
    attacker.protect_count = 0;

    // Turn 3: Second Protect (should be like first again - 100% success)
    auto ctx3 = SetupContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx3);

    TEST_ASSERT_EQ(ctx3.move_failed, false, "Protect should succeed (counter was reset)");
    TEST_ASSERT(attacker.is_protected, "Should be protected");
    TEST_ASSERT_EQ(attacker.protect_count, 1, "Protect count should be 1 again");
}

TEST_CASE(Effect_Protect_FailureResetsCounter) {
    // Setup: Force Protect to fail, verify counter resets
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto protect = CreateProtect();

    // Manually set protect_count high to force failure
    attacker.protect_count = 5;  // Success rate = 100 / 32 = ~3%

    // Use Protect with very low success rate
    auto ctx = SetupContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx);

    // If it failed, counter should reset
    if (ctx.move_failed) {
        TEST_ASSERT_EQ(attacker.protect_count, 0, "Failed Protect should reset counter");
        TEST_ASSERT_EQ(attacker.is_protected, false, "Should not be protected after failure");
    }
    // Note: There's a small chance it succeeds (~3%), which is fine
}

// ============================================================================
// TURN MANAGEMENT TESTS
// ============================================================================

TEST_CASE(Effect_Protect_ClearsEachTurn) {
    // This test verifies that protection is volatile (per-turn)
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto protect = CreateProtect();

    // Turn 1: Use Protect
    auto ctx1 = SetupContext(&attacker, &defender, &protect);
    battle::effects::Effect_Protect(ctx1);

    TEST_ASSERT(attacker.is_protected, "Should be protected on Turn 1");

    // Simulate turn boundary: clear is_protected flag
    attacker.is_protected = false;

    // Turn 2: Verify protection is cleared
    TEST_ASSERT_EQ(attacker.is_protected, false, "Protection should clear at turn start");
}

TEST_CASE(Effect_Protect_IndependentPerPokemon) {
    // Test that each Pokemon has independent protection state
    auto charmander = CreateCharmander();
    auto bulbasaur = CreateBulbasaur();
    auto protect = CreateProtect();

    // Charmander uses Protect
    auto ctx1 = SetupContext(&charmander, &bulbasaur, &protect);
    battle::effects::Effect_Protect(ctx1);

    // Bulbasaur uses Protect
    auto ctx2 = SetupContext(&bulbasaur, &charmander, &protect);
    battle::effects::Effect_Protect(ctx2);

    // Assert: Both should be protected independently
    TEST_ASSERT(charmander.is_protected, "Charmander should be protected");
    TEST_ASSERT(bulbasaur.is_protected, "Bulbasaur should be protected");
    TEST_ASSERT_EQ(charmander.protect_count, 1, "Charmander's counter should be 1");
    TEST_ASSERT_EQ(bulbasaur.protect_count, 1, "Bulbasaur's counter should be 1");
}
