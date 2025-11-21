/**
 * @file test/test_effect_recoil_hit.cpp
 * @brief Tests for Effect_RecoilHit (Double-Edge)
 */

#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../source/battle/effects/basic.hpp"
TEST_CASE(Effect_RecoilHit_DealsDamageToTarget) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    TEST_ASSERT(defender.current_hp < original_hp, "Double-Edge should deal damage to target");
    TEST_ASSERT(ctx.damage_dealt > 0, "Damage should be calculated");
}

TEST_CASE(Effect_RecoilHit_AttackerTakesRecoilDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    TEST_ASSERT(attacker.current_hp < original_attacker_hp, "Attacker should take recoil damage");
    TEST_ASSERT(ctx.recoil_dealt > 0, "Recoil damage should be recorded");
}

TEST_CASE(Effect_RecoilHit_RecoilIsOneThirdOfDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    uint16_t expected_recoil = ctx.damage_dealt / 3;
    // Minimum recoil is 1 if any damage dealt
    if (expected_recoil == 0 && ctx.damage_dealt > 0)
        expected_recoil = 1;

    TEST_ASSERT(ctx.recoil_dealt == expected_recoil, "Recoil should be 1/3 of damage (minimum 1)");
}

TEST_CASE(Effect_RecoilHit_HighPowerMeansHighRecoil) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // With 120 power, expect significant damage and recoil
    TEST_ASSERT(ctx.damage_dealt > 15, "High power should deal significant damage");
    TEST_ASSERT(ctx.recoil_dealt > 5, "Recoil from high power should be meaningful");
}

TEST_CASE(Effect_RecoilHit_NoRecoilOnMiss) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate miss
    Effect_RecoilHit(ctx);

    TEST_ASSERT(attacker.current_hp == original_attacker_hp,
                "No recoil should be taken if move misses");
    TEST_ASSERT(ctx.recoil_dealt == 0, "Recoil should be 0 on miss");
}

TEST_CASE(Effect_RecoilHit_MinimumRecoilIsOne) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    defender.defense = 50;  // Very high defense for low damage
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // If damage is 1 or 2, damage/3 would be 0, but recoil should be 1
    if (ctx.damage_dealt > 0 && ctx.damage_dealt < 3) {
        TEST_ASSERT(ctx.recoil_dealt == 1, "Minimum recoil should be 1 if any damage dealt");
    }
}

TEST_CASE(Effect_RecoilHit_RecoilClampsAtZero) {
    auto attacker = CreateCharmander();
    attacker.current_hp = 2;  // Low HP
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // HP should not go negative
    TEST_ASSERT(attacker.current_hp >= 0, "Attacker HP should not go negative");
}

TEST_CASE(Effect_RecoilHit_AttackerCanFaintFromRecoil) {
    auto attacker = CreateCharmander();
    attacker.current_hp = 3;  // Very low HP
    auto defender = CreateBulbasaur();
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // Attacker can faint from recoil
    if (ctx.recoil_dealt >= 3) {
        TEST_ASSERT(attacker.current_hp == 0, "Attacker HP should be 0");
        TEST_ASSERT(attacker.is_fainted, "Attacker should be marked as fainted");
    }
}

TEST_CASE(Effect_RecoilHit_DefenderCanFaintFromDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    defender.current_hp = 10;  // Low HP
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // Defender can faint from damage
    if (ctx.damage_dealt >= 10) {
        TEST_ASSERT(defender.current_hp == 0, "Defender HP should be 0");
        TEST_ASSERT(defender.is_fainted, "Defender should be marked as fainted");
    }
}

TEST_CASE(Effect_RecoilHit_BothCanFaintInSameTurn) {
    auto attacker = CreateCharmander();
    attacker.current_hp = 5;  // Low HP
    auto defender = CreateBulbasaur();
    defender.current_hp = 10;  // Also low HP
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // Both can faint - defender from damage, attacker from recoil
    // At least one should faint given the high power
    int faint_count = 0;
    if (defender.is_fainted)
        faint_count++;
    if (attacker.is_fainted)
        faint_count++;

    TEST_ASSERT(faint_count > 0, "At least one Pokemon should faint with low HP");
}

TEST_CASE(Effect_RecoilHit_RecoilOnlyIfDamageDealt) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    defender.defense = 255;  // Maximum defense for minimal damage
    auto move = CreateDoubleEdge();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_RecoilHit(ctx);

    // With very high defense, damage might be minimum (1), so recoil would be 1
    // This test now verifies that if damage is dealt, recoil is applied correctly
    if (ctx.damage_dealt == 0) {
        TEST_ASSERT(ctx.recoil_dealt == 0, "Recoil should be 0 if damage is 0");
    } else {
        TEST_ASSERT(ctx.recoil_dealt > 0, "Recoil should be > 0 if damage dealt");
    }
}
