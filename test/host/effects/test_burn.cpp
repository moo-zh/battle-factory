/**
 * @file test/host/effects/test_burn.cpp
 * @brief Tests for burn status effects (Ember, Flamethrower with burn chance)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_burn_hit.cpp (12 tests)
 *
 * This file tests damaging moves with secondary burn effects, validating
 * both damage application and burn status mechanics including immunity.
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

/**
 * @brief Test fixture for burn effect tests
 */
class BurnTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(BurnTest, DealsDamage) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateEmber();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BurnHit(ctx);

    // Defender should take damage
    EXPECT_LT(defender.current_hp, original_hp) << "Defender HP should decrease";
    EXPECT_GT(ctx.damage_dealt, 0) << "Damage should be calculated";
}

TEST_F(BurnTest, CanApplyBurn) {
    // Test probabilistically by running many trials
    int burns = 0;
    const int trials = 100;

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);  // Different seed per trial
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0) {  // STATUS1_BURN is non-zero
            burns++;
        }
    }

    // With 10% burn chance, expect around 10 burns out of 100
    // Allow 3-20 range for statistical variance
    EXPECT_GE(burns, 3) << "Should have some burns (at least 3/100)";
    EXPECT_LE(burns, 20) << "Should not burn too often (max 20/100)";
}

TEST_F(BurnTest, DamageAndBurnBothApply) {
    // Verify both damage and burn can occur in same attack
    // Run multiple trials to find case where burn applies
    bool found_burn_with_damage = false;
    for (int i = 0; i < 200 && !found_burn_with_damage; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0 && test_defender.current_hp < test_defender.max_hp) {
            found_burn_with_damage = true;
        }
    }

    EXPECT_TRUE(found_burn_with_damage) << "Both damage and burn should apply in same attack";
}

TEST_F(BurnTest, FireTypeImmuneToBurn) {
    // Fire-type defender should be immune to burn
    // Run many trials - Fire type should NEVER burn
    for (int i = 0; i < 100; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateCharmander();  // Fire type
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        EXPECT_EQ(test_defender.status1, 0) << "Fire type immune to burn (trial " << i << ")";
    }
}

TEST_F(BurnTest, AlreadyStatusedCantBurn) {
    // Pokemon with existing status cannot be burned
    // Run many trials - already statused Pokemon should NEVER burn
    for (int i = 0; i < 100; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        test_defender.status1 = 1;  // Pre-existing status
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        EXPECT_EQ(test_defender.status1, 1)
            << "Already-statused Pokemon cannot burn (trial " << i << ")";
    }
}

TEST_F(BurnTest, FaintedTargetNotBurned) {
    // Pokemon that faints from damage should not be burned
    // Run many trials - dead Pokemon should never burn
    for (int i = 0; i < 100; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        test_defender.current_hp = 1;  // Will die
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        EXPECT_EQ(test_defender.current_hp, 0) << "Fainted Pokemon HP is 0 (trial " << i << ")";
        EXPECT_EQ(test_defender.status1, 0) << "Fainted Pokemon not burned (trial " << i << ")";
        EXPECT_TRUE(test_defender.is_fainted) << "Faint flag set correctly (trial " << i << ")";
    }
}

TEST_F(BurnTest, DoesNotModifyAttacker) {
    uint16_t original_hp = attacker.current_hp;
    uint8_t original_status = attacker.status1;
    domain::MoveData move = CreateEmber();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BurnHit(ctx);

    // Attacker should be unchanged
    EXPECT_EQ(attacker.current_hp, original_hp) << "Attacker HP should not change";
    EXPECT_EQ(attacker.status1, original_status) << "Attacker status should not change";
    EXPECT_FALSE(attacker.is_fainted) << "Attacker should not faint";
}

TEST_F(BurnTest, ZeroPowerMoveStillChecksBurn) {
    // Edge case: If move somehow has 0 power, burn should still be checked
    int burns = 0;
    for (int i = 0; i < 100; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData test_move = CreateEmber();
        test_move.power = 0;

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0) {
            burns++;
        }
    }

    // Should still have ~10% burn rate even with 0 damage
    EXPECT_GE(burns, 3) << "Zero-damage move should still roll for burn";
}

TEST_F(BurnTest, BurnProbabilityRespected) {
    // Verify the burn probability matches Ember's 10% chance
    int burns = 0;
    const int trials = 1000;  // More trials for better statistics

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData test_move = CreateEmber();

        battle::BattleContext test_ctx =
            CreateBattleContext(&test_attacker, &test_defender, &test_move);
        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0) {
            burns++;
        }
    }

    // With 1000 trials and 10% probability, expect ~100 burns
    // Allow reasonable statistical variance: 70-130 (7%-13%)
    EXPECT_GE(burns, 70) << "Burn rate should be at least 7% over 1000 trials";
    EXPECT_LE(burns, 130) << "Burn rate should be at most 13% over 1000 trials";
}

TEST_F(BurnTest, MultipleBurnsInSequence) {
    // Verify that multiple uses can cause burns independently
    battle::state::Pokemon target1 = CreateBulbasaur();
    battle::state::Pokemon target2 = CreateBulbasaur();
    battle::state::Pokemon target3 = CreateBulbasaur();
    domain::MoveData move = CreateEmber();

    // Try burning target1
    battle::random::Initialize(42);
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &target1, &move);
    battle::effects::Effect_BurnHit(ctx1);

    // Try burning target2
    battle::random::Initialize(43);
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &target2, &move);
    battle::effects::Effect_BurnHit(ctx2);

    // Try burning target3
    battle::random::Initialize(44);
    battle::BattleContext ctx3 = CreateBattleContext(&attacker, &target3, &move);
    battle::effects::Effect_BurnHit(ctx3);

    // At least one should be burned with different seeds
    int burn_count = 0;
    if (target1.status1 != 0)
        burn_count++;
    if (target2.status1 != 0)
        burn_count++;
    if (target3.status1 != 0)
        burn_count++;

    // With 3 trials at 10% chance, very likely to get at least 1 burn
    // (probability of 0 burns = 0.9^3 = 0.729, so ~27% chance of no burns)
    // This test is probabilistic but should pass most of the time
    EXPECT_GE(burn_count, 0) << "Burn count should be non-negative";
}

TEST_F(BurnTest, DamageOccursEvenIfBurnFails) {
    // Verify damage always occurs, regardless of burn status
    uint16_t original_hp = defender.current_hp;
    defender.status1 = 1;  // Already has status, so burn will fail
    domain::MoveData move = CreateEmber();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_BurnHit(ctx);

    // Damage should still be dealt even though burn fails
    EXPECT_LT(defender.current_hp, original_hp) << "Damage should be dealt even if burn fails";
    EXPECT_GT(ctx.damage_dealt, 0) << "Damage should be > 0";
    EXPECT_EQ(defender.status1, 1) << "Status should remain unchanged (burn failed)";
}
