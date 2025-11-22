/**
 * @file test/unit/effects/test_baton_pass.cpp
 * @brief Tests for Effect_BatonPass (Baton Pass - transfers stat stages)
 */

#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "../../../source/domain/stats.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// ============================================================================
// BASIC STAT TRANSFER
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersAllStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Set up attacker with various stat stages
    attacker.stat_stages[STAT_ATK] = +2;
    attacker.stat_stages[STAT_DEF] = +1;
    attacker.stat_stages[STAT_SPEED] = -1;
    attacker.stat_stages[STAT_SPATK] = +3;
    attacker.stat_stages[STAT_SPDEF] = -2;
    attacker.stat_stages[STAT_ACC] = +1;
    attacker.stat_stages[STAT_EVASION] = 0;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == +2, "ATK should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == +1, "DEF should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -1, "SPEED should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == +3, "SPATK should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -2, "SPDEF should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_ACC] == +1, "ACC should be transferred");
    TEST_ASSERT(defender.stat_stages[STAT_EVASION] == 0, "EVASION should be transferred");
}

TEST_CASE(Effect_BatonPass_AlwaysSucceeds) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(!ctx.move_failed, "Baton Pass should always succeed");
}

// ============================================================================
// POSITIVE STAT STAGES
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersMaxPositiveStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Set all stats to +6 (max boost)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = +6;
    }

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        TEST_ASSERT(defender.stat_stages[i] == +6, "All stats should be +6");
    }
}

TEST_CASE(Effect_BatonPass_TransfersSinglePositiveStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_ATK] = +4;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == +4, "ATK should be +4");
    // Other stats should be 0 (neutral)
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "DEF should be 0");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == 0, "SPEED should be 0");
}

// ============================================================================
// NEGATIVE STAT STAGES
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_SPEED] = -3;
    attacker.stat_stages[STAT_ATK] = -1;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -3, "SPEED should be -3");
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -1, "ATK should be -1");
}

TEST_CASE(Effect_BatonPass_TransfersMaxNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Set all stats to -6 (max drop)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = -6;
    }

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        TEST_ASSERT(defender.stat_stages[i] == -6, "All stats should be -6");
    }
}

// ============================================================================
// OVERWRITING EXISTING STAGES
// ============================================================================

TEST_CASE(Effect_BatonPass_OverwritesExistingStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Defender starts with +2 ATK
    defender.stat_stages[STAT_ATK] = +2;

    // Attacker has -1 ATK
    attacker.stat_stages[STAT_ATK] = -1;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -1,
                "ATK should be overwritten to -1 (not added)");
}

TEST_CASE(Effect_BatonPass_OverwritesAllExistingStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Defender has various stat boosts
    defender.stat_stages[STAT_ATK] = +3;
    defender.stat_stages[STAT_DEF] = +2;
    defender.stat_stages[STAT_SPEED] = -1;

    // Attacker has different values
    attacker.stat_stages[STAT_ATK] = -2;
    attacker.stat_stages[STAT_DEF] = 0;
    attacker.stat_stages[STAT_SPEED] = +4;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -2, "ATK should be -2 (overwritten)");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "DEF should be 0 (overwritten)");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == +4, "SPEED should be +4 (overwritten)");
}

// ============================================================================
// NEUTRAL STAGES
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersNeutralStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Attacker has all neutral stages (0)
    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        attacker.stat_stages[i] = 0;
    }

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    for (int i = 0; i < NUM_BATTLE_STATS; i++) {
        TEST_ASSERT(defender.stat_stages[i] == 0, "All stats should be 0 (neutral)");
    }
}

// ============================================================================
// MIXED STAGES
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersMixedStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    // Set up complex mixed stat stages
    attacker.stat_stages[STAT_ATK] = +6;     // Max boost
    attacker.stat_stages[STAT_DEF] = +4;     // High boost
    attacker.stat_stages[STAT_SPEED] = -6;   // Max drop
    attacker.stat_stages[STAT_SPATK] = +2;   // Moderate boost
    attacker.stat_stages[STAT_SPDEF] = -3;   // Moderate drop
    attacker.stat_stages[STAT_ACC] = +1;     // Small boost
    attacker.stat_stages[STAT_EVASION] = 0;  // Neutral

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == +6, "ATK should be +6");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == +4, "DEF should be +4");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -6, "SPEED should be -6");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == +2, "SPATK should be +2");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -3, "SPDEF should be -3");
    TEST_ASSERT(defender.stat_stages[STAT_ACC] == +1, "ACC should be +1");
    TEST_ASSERT(defender.stat_stages[STAT_EVASION] == 0, "EVASION should be 0");
}

// ============================================================================
// SPECIFIC STAT FOCUS
// ============================================================================

TEST_CASE(Effect_BatonPass_TransfersAttack) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_ATK] = +5;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == +5, "ATK should be +5");
}

TEST_CASE(Effect_BatonPass_TransfersDefense) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_DEF] = +3;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_DEF] == +3, "DEF should be +3");
}

TEST_CASE(Effect_BatonPass_TransfersSpeed) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_SPEED] = +6;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == +6, "SPEED should be +6");
}

TEST_CASE(Effect_BatonPass_TransfersSpecialAttack) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_SPATK] = +4;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == +4, "SPATK should be +4");
}

TEST_CASE(Effect_BatonPass_TransfersSpecialDefense) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_SPDEF] = +2;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == +2, "SPDEF should be +2");
}

TEST_CASE(Effect_BatonPass_TransfersAccuracy) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_ACC] = -2;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ACC] == -2, "ACC should be -2");
}

TEST_CASE(Effect_BatonPass_TransfersEvasion) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateBatonPass();

    attacker.stat_stages[STAT_EVASION] = +3;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_BatonPass(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_EVASION] == +3, "EVASION should be +3");
}

// ============================================================================
// SEQUENTIAL BATON PASSES
// ============================================================================

TEST_CASE(Effect_BatonPass_CanChainMultipleTimes) {
    auto pokemon1 = CreateBulbasaur();
    auto pokemon2 = CreateCharmander();
    auto pokemon3 = CreatePikachu();
    auto move = CreateBatonPass();

    // Pokemon 1 has +3 ATK
    pokemon1.stat_stages[STAT_ATK] = +3;

    // Pass from Pokemon 1 to Pokemon 2
    BattleContext ctx1 = SetupContext(&pokemon1, &pokemon2, &move);
    Effect_BatonPass(ctx1);
    TEST_ASSERT(pokemon2.stat_stages[STAT_ATK] == +3, "Pokemon 2 should have +3 ATK");

    // Pass from Pokemon 2 to Pokemon 3
    BattleContext ctx2 = SetupContext(&pokemon2, &pokemon3, &move);
    Effect_BatonPass(ctx2);
    TEST_ASSERT(pokemon3.stat_stages[STAT_ATK] == +3, "Pokemon 3 should have +3 ATK");
}
