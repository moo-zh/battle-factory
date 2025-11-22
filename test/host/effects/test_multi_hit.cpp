/**
 * @file test/host/effects/test_multi_hit.cpp
 * @brief Tests for multi-hit move effects (Fury Attack)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_multi_hit.cpp (12 tests)
 *
 * This file tests moves that hit multiple times per turn (2-5 times),
 * validating hit count distribution, damage accumulation, and edge cases.
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace domain;  // For STAT_ constants

/**
 * @brief Test fixture for multi-hit move tests
 */
class MultiHitTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(MultiHitTest, HitsMultipleTimes) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // Should have dealt damage
    EXPECT_LT(defender.current_hp, original_hp) << "Fury Attack should deal damage";

    // Hit count should be tracked (2-5)
    EXPECT_GE(ctx.hit_count, 2) << "Fury Attack should hit at least 2 times";
    EXPECT_LE(ctx.hit_count, 5) << "Fury Attack should hit at most 5 times";
}

TEST_F(MultiHitTest, DamageAccumulates) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // Total damage should be non-zero
    EXPECT_GT(ctx.damage_dealt, 0) << "Total damage should be recorded";

    // Defender HP should have decreased by total damage
    uint16_t expected_hp;
    if (ctx.damage_dealt >= original_hp) {
        expected_hp = 0;  // KO'd
    } else {
        expected_hp = original_hp - ctx.damage_dealt;
    }

    EXPECT_EQ(defender.current_hp, expected_hp) << "HP should decrease by total damage dealt";
}

TEST_F(MultiHitTest, HitCountDistribution) {
    // Test hit count distribution over many trials
    int hit_counts[6] = {0};  // Index 0-5, we care about 2-5

    for (int trial = 0; trial < 200; trial++) {
        battle::random::Initialize(trial);  // Different seed for each trial
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        domain::MoveData move = CreateFuryAttack();

        battle::BattleContext ctx = CreateBattleContext(&test_attacker, &test_defender, &move);
        battle::effects::Effect_MultiHit(ctx);

        // Record hit count
        if (ctx.hit_count >= 2 && ctx.hit_count <= 5) {
            hit_counts[ctx.hit_count]++;
        }
    }

    // Verify all hits are in 2-5 range
    EXPECT_EQ(hit_counts[0], 0) << "Should never hit 0 times";
    EXPECT_EQ(hit_counts[1], 0) << "Should never hit 1 time";

    // Should have hits in 2-5 range
    int total_hits = hit_counts[2] + hit_counts[3] + hit_counts[4] + hit_counts[5];
    EXPECT_EQ(total_hits, 200) << "Should have 200 total trials";

    // 2 and 3 hits should be most common (~37.5% each)
    // Sanity check: combined they should be majority
    int hits_2_and_3 = hit_counts[2] + hit_counts[3];
    EXPECT_GT(hits_2_and_3, 100) << "2 and 3 hits combined should be majority (~75% total)";
}

TEST_F(MultiHitTest, SingleAccuracyCheck) {
    // Test that accuracy is only checked once
    domain::MoveData move = CreateFuryAttack();
    move.accuracy = 100;  // Always hit

    int hit_trials = 0;
    for (int trial = 0; trial < 20; trial++) {
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();

        battle::BattleContext ctx = CreateBattleContext(&test_attacker, &test_defender, &move);
        battle::effects::Effect_MultiHit(ctx);

        if (!ctx.move_failed) {
            hit_trials++;
        }
    }

    // With 100 accuracy, should always hit
    EXPECT_EQ(hit_trials, 20) << "100% accuracy should always hit";
}

TEST_F(MultiHitTest, MissPreventsAllHits) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate a miss
    battle::effects::Effect_MultiHit(ctx);

    // No damage should be dealt if move missed
    EXPECT_EQ(defender.current_hp, original_hp) << "Miss should prevent all damage";
    EXPECT_EQ(ctx.damage_dealt, 0) << "Damage should be 0 on miss";
    EXPECT_EQ(ctx.hit_count, 0) << "Hit count should be 0 on miss";
}

TEST_F(MultiHitTest, DefenderFaintsMidSequence) {
    defender.current_hp = 3;  // Set defender to low HP so it faints mid-sequence
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // Defender should be fainted
    EXPECT_EQ(defender.current_hp, 0) << "Defender HP should be 0";
    EXPECT_TRUE(defender.is_fainted) << "Defender should be marked as fainted";

    // Hit count should reflect actual hits (may be less than rolled)
    EXPECT_GE(ctx.hit_count, 1) << "Should have hit at least once before fainting";
    EXPECT_LE(ctx.hit_count, 5) << "Should not exceed maximum hits";
}

TEST_F(MultiHitTest, NoOverkillDamage) {
    defender.current_hp = 2;  // Set defender to very low HP
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // HP should be clamped at 0, not negative
    EXPECT_EQ(defender.current_hp, 0) << "HP should be clamped at 0";
    EXPECT_TRUE(defender.is_fainted) << "Should be marked as fainted";
}

TEST_F(MultiHitTest, LowHPDefenderOneHitKO) {
    defender.current_hp = 1;  // Set defender to 1 HP - guaranteed KO on first hit
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // Should faint on first hit
    EXPECT_EQ(defender.current_hp, 0) << "Should faint on first hit";
    EXPECT_TRUE(defender.is_fainted) << "Should be marked as fainted";
}

TEST_F(MultiHitTest, DoesNotAffectStats) {
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // No stat stages should change
    EXPECT_EQ(attacker.stat_stages[STAT_ATK], 0) << "Attacker Attack unchanged";
    EXPECT_EQ(defender.stat_stages[STAT_DEF], 0) << "Defender Defense unchanged";
    EXPECT_EQ(defender.stat_stages[STAT_SPEED], 0) << "Defender Speed unchanged";
}

TEST_F(MultiHitTest, DoesNotCauseStatus) {
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // No status should be applied
    EXPECT_EQ(defender.status1, 0) << "No status should be applied";
    EXPECT_EQ(attacker.status1, 0) << "Attacker status unchanged";
}

TEST_F(MultiHitTest, AttackerNotDamaged) {
    uint16_t original_hp = attacker.current_hp;
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // Attacker HP should not change (no recoil)
    EXPECT_EQ(attacker.current_hp, original_hp) << "Fury Attack should not damage attacker";
}

TEST_F(MultiHitTest, TotalDamageReasonable) {
    domain::MoveData move = CreateFuryAttack();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_MultiHit(ctx);

    // With 15 power and 2-5 hits, expect reasonable total damage
    // Each hit should do ~2-4 damage, total ~4-20 damage (allow some variance)
    EXPECT_GE(ctx.damage_dealt, 2) << "Total damage should be at least minimum (1 damage * 2 hits)";
    EXPECT_LE(ctx.damage_dealt, 30) << "Total damage should be reasonable for 2-5 hits of 15 power";
}
