/**
 * @file test/test_effect_drain_hit.cpp
 * @brief Tests for Effect_DrainHit (Giga Drain)
 */

#include "../../../src/battle/effects/basic.hpp"
#include "../../../src/battle/state/pokemon.hpp"
#include "../../../src/domain/move.hpp"
#include "../../../src/domain/species.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../src/battle/effects/basic.hpp"
TEST_CASE(Effect_DrainHit_DealsDamageToTarget) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    TEST_ASSERT(defender.current_hp < original_hp, "Giga Drain should deal damage to target");
    TEST_ASSERT(ctx.damage_dealt > 0, "Damage should be calculated");
}

TEST_CASE(Effect_DrainHit_AttackerHealsFromDrain) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    TEST_ASSERT(attacker.current_hp > original_attacker_hp, "Attacker should heal from drain");
    TEST_ASSERT(ctx.drain_received > 0, "Drain amount should be recorded");
}

TEST_CASE(Effect_DrainHit_DrainIsHalfOfDamage) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 5;  // Low HP to avoid clamping
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    uint16_t drain_received = attacker.current_hp - original_attacker_hp;
    uint16_t expected_drain = ctx.damage_dealt / 2;

    // Minimum drain is 1 if any damage dealt
    if (expected_drain == 0 && ctx.damage_dealt > 0)
        expected_drain = 1;

    TEST_ASSERT(drain_received == expected_drain, "Drain should be 1/2 of damage (minimum 1)");
    TEST_ASSERT(ctx.drain_received == expected_drain, "Context should record correct drain amount");
}

TEST_CASE(Effect_DrainHit_ModeratePowerModerateDrain) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // With 60 power, expect moderate damage and drain
    TEST_ASSERT(ctx.damage_dealt > 8, "Moderate power should deal reasonable damage");
    TEST_ASSERT(ctx.drain_received > 4, "Drain from moderate power should be meaningful");
}

TEST_CASE(Effect_DrainHit_NoDrainOnMiss) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();
    uint16_t original_attacker_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate miss
    Effect_DrainHit(ctx);

    TEST_ASSERT(attacker.current_hp == original_attacker_hp,
                "No drain should occur if move misses");
    TEST_ASSERT(ctx.drain_received == 0, "Drain should be 0 on miss");
}

TEST_CASE(Effect_DrainHit_MinimumDrainIsOne) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    auto defender = CreateCharmander();
    defender.defense = 50;  // Very high defense for low damage
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // If damage is 1, damage/2 would be 0, but drain should be 1
    if (ctx.damage_dealt > 0 && ctx.damage_dealt < 2) {
        TEST_ASSERT(ctx.drain_received == 1, "Minimum drain should be 1 if any damage dealt");
    }
}

TEST_CASE(Effect_DrainHit_CannotOverheal) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = attacker.max_hp - 2;  // Almost full HP
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // HP should not exceed max_hp
    TEST_ASSERT(attacker.current_hp <= attacker.max_hp, "HP should not exceed max_hp");
    TEST_ASSERT(attacker.current_hp == attacker.max_hp, "HP should be clamped to max_hp");
}

TEST_CASE(Effect_DrainHit_FullHPStillHeals) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = attacker.max_hp;  // Already full HP
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // Still processes drain, just clamped to max
    TEST_ASSERT(attacker.current_hp == attacker.max_hp, "HP should remain at max_hp");
    TEST_ASSERT(ctx.drain_received > 0, "Drain should still be calculated even at full HP");
}

TEST_CASE(Effect_DrainHit_DrainClampsAtMaxHP) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = attacker.max_hp - 3;  // 3 HP below max
    auto defender = CreateCharmander();
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // Even if drain would heal more than 3, HP should clamp at max_hp
    TEST_ASSERT(attacker.current_hp == attacker.max_hp, "HP should be clamped to max_hp");
}

TEST_CASE(Effect_DrainHit_DefenderCanFaintFromDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    defender.current_hp = 8;  // Low HP
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // Defender can faint from damage
    if (ctx.damage_dealt >= 8) {
        TEST_ASSERT(defender.current_hp == 0, "Defender HP should be 0");
        TEST_ASSERT(defender.is_fainted, "Defender should be marked as fainted");
    }
}

TEST_CASE(Effect_DrainHit_AttackerStillHealsWhenDefenderFaints) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    auto defender = CreateCharmander();
    defender.current_hp = 5;  // Very low HP, will likely faint
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // Even if defender faints, attacker should heal
    TEST_ASSERT(attacker.current_hp > original_attacker_hp,
                "Attacker should heal even when defender faints");
    TEST_ASSERT(ctx.drain_received > 0, "Drain should be calculated");
}

TEST_CASE(Effect_DrainHit_DrainOnlyIfDamageDealt) {
    auto attacker = CreateBulbasaur();
    attacker.current_hp = 10;  // Damaged attacker
    uint16_t original_attacker_hp = attacker.current_hp;
    auto defender = CreateCharmander();
    defender.defense = 255;  // Maximum defense for minimal damage
    auto move = CreateGigaDrain();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_DrainHit(ctx);

    // If no damage dealt, no drain
    if (ctx.damage_dealt == 0) {
        TEST_ASSERT(ctx.drain_received == 0, "Drain should be 0 if damage is 0");
        TEST_ASSERT(attacker.current_hp == original_attacker_hp,
                    "Attacker HP should not change if no damage dealt");
    } else {
        TEST_ASSERT(ctx.drain_received > 0, "Drain should be > 0 if damage dealt");
        TEST_ASSERT(attacker.current_hp > original_attacker_hp,
                    "Attacker HP should increase if damage dealt");
    }
}
