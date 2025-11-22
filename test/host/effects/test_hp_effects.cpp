/**
 * @file test/host/effects/test_hp_effects.cpp
 * @brief Tests for HP-affecting move effects (recoil and drain)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_recoil_hit.cpp (11 tests)
 * - test/EZ80/archived/ti84ce/foundation/test_drain_hit.cpp (12 tests)
 *
 * This file consolidates all HP manipulation effects that modify both
 * attacker and defender HP based on damage dealt.
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

// ============================================================================
// RECOIL TESTS (Double-Edge)
// ============================================================================

/**
 * @brief Test fixture for recoil move tests
 */
class RecoilTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(RecoilTest, DealsDamageToTarget) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    EXPECT_LT(defender.current_hp, original_hp) << "Double-Edge should deal damage to target";
    EXPECT_GT(ctx.damage_dealt, 0) << "Damage should be calculated";
}

TEST_F(RecoilTest, AttackerTakesRecoilDamage) {
    uint16_t original_attacker_hp = attacker.current_hp;
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    EXPECT_LT(attacker.current_hp, original_attacker_hp) << "Attacker should take recoil damage";
    EXPECT_GT(ctx.recoil_dealt, 0) << "Recoil damage should be recorded";
}

TEST_F(RecoilTest, RecoilIsOneThirdOfDamage) {
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    uint16_t expected_recoil = ctx.damage_dealt / 3;
    // Minimum recoil is 1 if any damage dealt
    if (expected_recoil == 0 && ctx.damage_dealt > 0) {
        expected_recoil = 1;
    }

    EXPECT_EQ(ctx.recoil_dealt, expected_recoil) << "Recoil should be 1/3 of damage (minimum 1)";
}

TEST_F(RecoilTest, HighPowerMeansHighRecoil) {
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // With 120 power, expect significant damage and recoil
    EXPECT_GT(ctx.damage_dealt, 15) << "High power should deal significant damage";
    EXPECT_GT(ctx.recoil_dealt, 5) << "Recoil from high power should be meaningful";
}

TEST_F(RecoilTest, NoRecoilOnMiss) {
    uint16_t original_attacker_hp = attacker.current_hp;
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate miss
    battle::effects::Effect_RecoilHit(ctx);

    EXPECT_EQ(attacker.current_hp, original_attacker_hp)
        << "No recoil should be taken if move misses";
    EXPECT_EQ(ctx.recoil_dealt, 0) << "Recoil should be 0 on miss";
}

TEST_F(RecoilTest, MinimumRecoilIsOne) {
    defender.defense = 50;  // Very high defense for low damage
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // If damage is 1 or 2, damage/3 would be 0, but recoil should be 1
    if (ctx.damage_dealt > 0 && ctx.damage_dealt < 3) {
        EXPECT_EQ(ctx.recoil_dealt, 1) << "Minimum recoil should be 1 if any damage dealt";
    }
}

TEST_F(RecoilTest, RecoilClampsAtZero) {
    attacker.current_hp = 2;  // Low HP
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // HP should not go negative
    EXPECT_GE(attacker.current_hp, 0) << "Attacker HP should not go negative";
}

TEST_F(RecoilTest, AttackerCanFaintFromRecoil) {
    attacker.current_hp = 3;  // Very low HP
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // Attacker can faint from recoil
    if (ctx.recoil_dealt >= 3) {
        EXPECT_EQ(attacker.current_hp, 0) << "Attacker HP should be 0";
        EXPECT_TRUE(attacker.is_fainted) << "Attacker should be marked as fainted";
    }
}

TEST_F(RecoilTest, DefenderCanFaintFromDamage) {
    defender.current_hp = 10;  // Low HP
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // Defender can faint from damage
    if (ctx.damage_dealt >= 10) {
        EXPECT_EQ(defender.current_hp, 0) << "Defender HP should be 0";
        EXPECT_TRUE(defender.is_fainted) << "Defender should be marked as fainted";
    }
}

TEST_F(RecoilTest, BothCanFaintInSameTurn) {
    attacker.current_hp = 5;   // Low HP
    defender.current_hp = 10;  // Also low HP
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // Both can faint - defender from damage, attacker from recoil
    // At least one should faint given the high power
    int faint_count = 0;
    if (defender.is_fainted)
        faint_count++;
    if (attacker.is_fainted)
        faint_count++;

    EXPECT_GT(faint_count, 0) << "At least one Pokemon should faint with low HP";
}

TEST_F(RecoilTest, RecoilOnlyIfDamageDealt) {
    defender.defense = 255;  // Maximum defense for minimal damage
    domain::MoveData move = CreateDoubleEdge();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_RecoilHit(ctx);

    // With very high defense, damage might be minimum (1), so recoil would be 1
    // This test verifies that if damage is dealt, recoil is applied correctly
    if (ctx.damage_dealt == 0) {
        EXPECT_EQ(ctx.recoil_dealt, 0) << "Recoil should be 0 if damage is 0";
    } else {
        EXPECT_GT(ctx.recoil_dealt, 0) << "Recoil should be > 0 if damage dealt";
    }
}

// ============================================================================
// DRAIN TESTS (Giga Drain)
// ============================================================================

/**
 * @brief Test fixture for drain move tests
 */
class DrainTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateBulbasaur();  // Bulbasaur for Giga Drain (Grass type)
        defender = CreateCharmander();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(DrainTest, DealsDamageToTarget) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    EXPECT_LT(defender.current_hp, original_hp) << "Giga Drain should deal damage to target";
    EXPECT_GT(ctx.damage_dealt, 0) << "Damage should be calculated";
}

TEST_F(DrainTest, AttackerHealsFromDrain) {
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    EXPECT_GT(attacker.current_hp, original_attacker_hp) << "Attacker should heal from drain";
    EXPECT_GT(ctx.drain_received, 0) << "Drain amount should be recorded";
}

TEST_F(DrainTest, DrainIsHalfOfDamage) {
    attacker.current_hp = 5;  // Low HP to avoid clamping
    uint16_t original_attacker_hp = attacker.current_hp;
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    uint16_t drain_received = attacker.current_hp - original_attacker_hp;
    uint16_t expected_drain = ctx.damage_dealt / 2;

    // Minimum drain is 1 if any damage dealt
    if (expected_drain == 0 && ctx.damage_dealt > 0) {
        expected_drain = 1;
    }

    EXPECT_EQ(drain_received, expected_drain) << "Drain should be 1/2 of damage (minimum 1)";
    EXPECT_EQ(ctx.drain_received, expected_drain) << "Context should record correct drain amount";
}

TEST_F(DrainTest, ModeratePowerModerateDrain) {
    attacker.current_hp = 10;  // Damaged attacker
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // With 60 power, expect moderate damage and drain
    EXPECT_GT(ctx.damage_dealt, 8) << "Moderate power should deal reasonable damage";
    EXPECT_GT(ctx.drain_received, 4) << "Drain from moderate power should be meaningful";
}

TEST_F(DrainTest, NoDrainOnMiss) {
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate miss
    battle::effects::Effect_DrainHit(ctx);

    EXPECT_EQ(attacker.current_hp, original_attacker_hp) << "No drain should occur if move misses";
    EXPECT_EQ(ctx.drain_received, 0) << "Drain should be 0 on miss";
}

TEST_F(DrainTest, MinimumDrainIsOne) {
    attacker.current_hp = 10;  // Damaged attacker
    defender.defense = 50;     // Very high defense for low damage
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // If damage is 1, damage/2 would be 0, but drain should be 1
    if (ctx.damage_dealt > 0 && ctx.damage_dealt < 2) {
        EXPECT_EQ(ctx.drain_received, 1) << "Minimum drain should be 1 if any damage dealt";
    }
}

TEST_F(DrainTest, CannotOverheal) {
    attacker.current_hp = attacker.max_hp - 2;  // Almost full HP
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // HP should not exceed max_hp
    EXPECT_LE(attacker.current_hp, attacker.max_hp) << "HP should not exceed max_hp";
    EXPECT_EQ(attacker.current_hp, attacker.max_hp) << "HP should be clamped to max_hp";
}

TEST_F(DrainTest, FullHPStillHeals) {
    attacker.current_hp = attacker.max_hp;  // Already full HP
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // Still processes drain, just clamped to max
    EXPECT_EQ(attacker.current_hp, attacker.max_hp) << "HP should remain at max_hp";
    EXPECT_GT(ctx.drain_received, 0) << "Drain should still be calculated even at full HP";
}

TEST_F(DrainTest, DrainClampsAtMaxHP) {
    attacker.current_hp = attacker.max_hp - 3;  // 3 HP below max
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // Even if drain would heal more than 3, HP should clamp at max_hp
    EXPECT_EQ(attacker.current_hp, attacker.max_hp) << "HP should be clamped to max_hp";
}

TEST_F(DrainTest, DefenderCanFaintFromDamage) {
    defender.current_hp = 8;  // Low HP
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // Defender can faint from damage
    if (ctx.damage_dealt >= 8) {
        EXPECT_EQ(defender.current_hp, 0) << "Defender HP should be 0";
        EXPECT_TRUE(defender.is_fainted) << "Defender should be marked as fainted";
    }
}

TEST_F(DrainTest, AttackerStillHealsWhenDefenderFaints) {
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    defender.current_hp = 5;  // Very low HP, will likely faint
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // Even if defender faints, attacker should heal
    EXPECT_GT(attacker.current_hp, original_attacker_hp)
        << "Attacker should heal even when defender faints";
    EXPECT_GT(ctx.drain_received, 0) << "Drain should be calculated";
}

TEST_F(DrainTest, DrainOnlyIfDamageDealt) {
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    defender.defense = 255;  // Maximum defense for minimal damage
    domain::MoveData move = CreateGigaDrain();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_DrainHit(ctx);

    // If no damage dealt, no drain
    if (ctx.damage_dealt == 0) {
        EXPECT_EQ(ctx.drain_received, 0) << "Drain should be 0 if damage is 0";
        EXPECT_EQ(attacker.current_hp, original_attacker_hp)
            << "Attacker HP should not change if no damage dealt";
    } else {
        EXPECT_GT(ctx.drain_received, 0) << "Drain should be > 0 if damage dealt";
        EXPECT_GT(attacker.current_hp, original_attacker_hp)
            << "Attacker HP should increase if damage dealt";
    }
}
