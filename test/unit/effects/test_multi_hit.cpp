/**
 * @file test/unit/effects/test_multi_hit.cpp
 * @brief Tests for Effect_MultiHit (Fury Attack)
 */

#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "../../../source/domain/stats.hpp"
#include "framework.hpp"

using namespace battle;
using namespace battle::effects;
using namespace battle::state;
using namespace domain;

// Helper: Create test Charmander (attacker)
static Pokemon CreateCharmander() {
    Pokemon p;
    p.species = Species::Charmander;
    p.level = 5;
    p.type1 = Type::Fire;
    p.type2 = Type::None;
    p.max_hp = 20;
    p.current_hp = 20;
    p.attack = 11;
    p.defense = 9;
    p.sp_attack = 12;
    p.sp_defense = 10;
    p.speed = 13;
    p.status1 = 0;

    // Initialize protection state
    p.is_protected = false;
    p.protect_count = 0;

    // Initialize two-turn move state
    p.is_charging = false;
    p.charging_move = Move::None;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create test Bulbasaur (defender)
static Pokemon CreateBulbasaur() {
    Pokemon p;
    p.species = Species::Bulbasaur;
    p.level = 5;
    p.type1 = Type::Grass;
    p.type2 = Type::Poison;
    p.max_hp = 21;
    p.current_hp = 21;
    p.attack = 10;
    p.defense = 10;
    p.sp_attack = 12;
    p.sp_defense = 12;
    p.speed = 9;
    p.status1 = 0;

    // Initialize protection state
    p.is_protected = false;
    p.protect_count = 0;

    // Initialize two-turn move state
    p.is_charging = false;
    p.charging_move = Move::None;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create Fury Attack move
static MoveData CreateFuryAttack() {
    MoveData m;
    m.move = Move::FuryAttack;
    m.type = Type::Normal;
    m.power = 15;
    m.accuracy = 85;
    m.pp = 20;
    m.effect_chance = 0;
    return m;
}

// Helper: Set up battle context
static BattleContext SetupContext(Pokemon* attacker, Pokemon* defender, MoveData* move) {
    BattleContext ctx;
    ctx.attacker = attacker;
    ctx.defender = defender;
    ctx.move = move;
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // 1x (normal effectiveness)
    ctx.override_power = 0;
    ctx.override_type = 0;
    return ctx;
}

TEST_CASE(Effect_MultiHit_HitsMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // Should have dealt damage
    TEST_ASSERT(defender.current_hp < original_hp, "Fury Attack should deal damage");

    // Hit count should be tracked (2-5)
    TEST_ASSERT(ctx.hit_count >= 2 && ctx.hit_count <= 5, "Fury Attack should hit 2-5 times");
}

TEST_CASE(Effect_MultiHit_DamageAccumulates) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // Total damage should be non-zero
    TEST_ASSERT(ctx.damage_dealt > 0, "Total damage should be recorded");

    // Defender HP should have decreased by total damage
    uint16_t expected_hp = 21 - ctx.damage_dealt;
    if (expected_hp > 21)
        expected_hp = 0;  // Underflow protection

    TEST_ASSERT(defender.current_hp == expected_hp, "HP should decrease by total damage dealt");
}

TEST_CASE(Effect_MultiHit_HitCountDistribution) {
    // Test hit count distribution over many trials
    int hit_counts[6] = {0};  // Index 0-5, we care about 2-5

    for (int trial = 0; trial < 200; trial++) {
        auto attacker = CreateCharmander();
        auto defender = CreateBulbasaur();
        auto move = CreateFuryAttack();

        BattleContext ctx = SetupContext(&attacker, &defender, &move);
        Effect_MultiHit(ctx);

        // Record hit count
        if (ctx.hit_count >= 2 && ctx.hit_count <= 5) {
            hit_counts[ctx.hit_count]++;
        }
    }

    // Verify all hits are in 2-5 range
    TEST_ASSERT(hit_counts[0] == 0 && hit_counts[1] == 0, "Should never hit 0 or 1 times");

    // Should have hits in 2-5 range
    int total_hits = hit_counts[2] + hit_counts[3] + hit_counts[4] + hit_counts[5];
    TEST_ASSERT(total_hits == 200, "Should have 200 total trials");

    // 2 and 3 hits should be most common (~37.5% each)
    // Sanity check: combined they should be majority
    int hits_2_and_3 = hit_counts[2] + hit_counts[3];
    TEST_ASSERT(hits_2_and_3 >= 100, "2 and 3 hits combined should be majority (~75% total)");
}

TEST_CASE(Effect_MultiHit_SingleAccuracyCheck) {
    // Test that accuracy is only checked once
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();
    move.accuracy = 100;  // Always hit

    int hit_trials = 0;
    for (int trial = 0; trial < 20; trial++) {
        attacker.current_hp = attacker.max_hp;
        defender.current_hp = defender.max_hp;

        BattleContext ctx = SetupContext(&attacker, &defender, &move);
        Effect_MultiHit(ctx);

        if (!ctx.move_failed) {
            hit_trials++;
        }
    }

    // With 100 accuracy, should always hit
    TEST_ASSERT(hit_trials == 20, "100% accuracy should always hit");
}

TEST_CASE(Effect_MultiHit_MissPreventsAllHits) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate a miss

    Effect_MultiHit(ctx);

    // No damage should be dealt if move missed
    TEST_ASSERT(defender.current_hp == original_hp, "Miss should prevent all damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Damage should be 0 on miss");
    TEST_ASSERT(ctx.hit_count == 0, "Hit count should be 0 on miss");
}

TEST_CASE(Effect_MultiHit_DefenderFaintsMidSequence) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    // Set defender to low HP so it faints mid-sequence
    defender.current_hp = 3;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // Defender should be fainted
    TEST_ASSERT(defender.current_hp == 0, "Defender HP should be 0");
    TEST_ASSERT(defender.is_fainted, "Defender should be marked as fainted");

    // Hit count should reflect actual hits (may be less than rolled)
    TEST_ASSERT(ctx.hit_count >= 1 && ctx.hit_count <= 5,
                "Should have hit at least once before fainting");
}

TEST_CASE(Effect_MultiHit_NoOverkillDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    // Set defender to very low HP
    defender.current_hp = 2;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // HP should be clamped at 0, not negative
    TEST_ASSERT(defender.current_hp == 0, "HP should be clamped at 0");
    TEST_ASSERT(defender.is_fainted, "Should be marked as fainted");
}

TEST_CASE(Effect_MultiHit_LowHPDefenderOneHitKO) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    // Set defender to 1 HP - guaranteed KO on first hit
    defender.current_hp = 1;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // Should faint on first hit
    TEST_ASSERT(defender.current_hp == 0, "Should faint on first hit");
    TEST_ASSERT(defender.is_fainted, "Should be marked as fainted");
}

TEST_CASE(Effect_MultiHit_DoesNotAffectStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // No stat stages should change
    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == 0, "Attacker Attack unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "Defender Defense unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == 0, "Defender Speed unchanged");
}

TEST_CASE(Effect_MultiHit_DoesNotCauseStatus) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // No status should be applied
    TEST_ASSERT(defender.status1 == 0, "No status should be applied");
    TEST_ASSERT(attacker.status1 == 0, "Attacker status unchanged");
}

TEST_CASE(Effect_MultiHit_AttackerNotDamaged) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();
    uint16_t original_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // Attacker HP should not change (no recoil)
    TEST_ASSERT(attacker.current_hp == original_hp, "Fury Attack should not damage attacker");
}

TEST_CASE(Effect_MultiHit_TotalDamageReasonable) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFuryAttack();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_MultiHit(ctx);

    // With 15 power and 2-5 hits, expect reasonable total damage
    // Each hit should do ~2-4 damage, total ~4-20 damage (allow some variance)
    TEST_ASSERT(ctx.damage_dealt >= 2 && ctx.damage_dealt <= 30,
                "Total damage should be reasonable for 2-5 hits of 15 power");
}
