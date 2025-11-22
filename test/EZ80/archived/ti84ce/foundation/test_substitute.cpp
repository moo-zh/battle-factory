/**
 * @file test/unit/effects/test_substitute.cpp
 * @brief Tests for Effect_Substitute (Substitute - creates HP-based decoy)
 */

#include "../../../src/battle/effects/basic.hpp"
#include "../../../src/battle/state/pokemon.hpp"
#include "../../../src/domain/move.hpp"
#include "../../../src/domain/species.hpp"
#include "../../../src/domain/stats.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

// ============================================================================
// BASIC SUBSTITUTE CREATION
// ============================================================================

TEST_CASE(Effect_Substitute_CreatesSuccessfully) {
    auto attacker = CreateBulbasaur();  // 45 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.has_substitute, "Substitute should be created");
    TEST_ASSERT(!ctx.move_failed, "Move should succeed");
}

TEST_CASE(Effect_Substitute_CostsCorrectHP) {
    auto attacker = CreateBulbasaur();  // 45 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    uint16_t original_hp = attacker.current_hp;    // 45
    uint16_t expected_cost = attacker.max_hp / 4;  // 45 / 4 = 11

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.current_hp == original_hp - expected_cost,
                "User HP should be reduced by 25% of max HP");
    TEST_ASSERT(attacker.current_hp == 34, "45 - 11 = 34 HP remaining");
}

TEST_CASE(Effect_Substitute_StoresCorrectSubHP) {
    auto attacker = CreateBulbasaur();  // 45 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    uint16_t expected_sub_hp = attacker.max_hp / 4;  // 11

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.substitute_hp == expected_sub_hp, "Substitute HP should be 25% of max HP");
    TEST_ASSERT(attacker.substitute_hp == 11, "45 / 4 = 11 substitute HP");
}

// ============================================================================
// HP COST CALCULATION
// ============================================================================

TEST_CASE(Effect_Substitute_RoundsDownCorrectly) {
    auto attacker = CreatePikachu();  // 35 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    // 35 / 4 = 8.75, rounds down to 8

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.substitute_hp == 8, "35 / 4 = 8 (rounded down)");
    TEST_ASSERT(attacker.current_hp == 27, "35 - 8 = 27 HP remaining");
}

TEST_CASE(Effect_Substitute_MinimumCost1HP) {
    // Create Pokemon with very low max HP
    auto attacker = CreateTestPokemon(domain::Species::Pikachu, domain::Type::Electric,
                                      domain::Type::None, 3, 50, 40, 50, 40, 90);
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    // 3 / 4 = 0, but minimum cost is 1

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.substitute_hp == 1, "Minimum substitute HP is 1");
    TEST_ASSERT(attacker.current_hp == 2, "3 - 1 = 2 HP remaining");
}

// ============================================================================
// FAILURE CASES
// ============================================================================

TEST_CASE(Effect_Substitute_FailsInsufficientHP) {
    auto attacker = CreateBulbasaur();  // 45 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    // Cost is 11 HP (45 / 4)
    // Set HP to exactly 11 (at threshold, should fail)
    attacker.current_hp = 11;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(ctx.move_failed, "Should fail with insufficient HP");
    TEST_ASSERT(!attacker.has_substitute, "No substitute should be created");
    TEST_ASSERT(attacker.current_hp == 11, "HP should be unchanged");
}

TEST_CASE(Effect_Substitute_FailsExactlyAtThreshold) {
    auto attacker = CreateCharmander();  // 39 HP
    auto defender = CreateBulbasaur();
    auto move = CreateSubstitute();
    uint16_t cost = attacker.max_hp / 4;  // 39 / 4 = 9
    attacker.current_hp = cost;           // Exactly at cost

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(ctx.move_failed, "Should fail when HP equals cost (need > cost)");
    TEST_ASSERT(!attacker.has_substitute, "No substitute created");
    TEST_ASSERT(attacker.current_hp == cost, "HP unchanged");
}

TEST_CASE(Effect_Substitute_FailsAlreadyHasSubstitute) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    // First substitute (should succeed)
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx1);
    TEST_ASSERT(attacker.has_substitute, "First substitute should succeed");

    // Second substitute attempt (should fail)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx2);
    TEST_ASSERT(ctx2.move_failed, "Should fail when already has substitute");
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_CASE(Effect_Substitute_SucceedsWithMinimalHP) {
    auto attacker = CreateBulbasaur();  // 45 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    uint16_t cost = attacker.max_hp / 4;  // 11
    // Set HP to cost + 1 (should succeed)
    attacker.current_hp = cost + 1;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(!ctx.move_failed, "Should succeed with HP > cost");
    TEST_ASSERT(attacker.has_substitute, "Substitute should be created");
    TEST_ASSERT(attacker.current_hp == 1, "12 - 11 = 1 HP remaining");
}

TEST_CASE(Effect_Substitute_CanRecreateAfterBreak) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    // Create first substitute
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx1);
    TEST_ASSERT(attacker.has_substitute, "First substitute created");

    // Simulate substitute breaking (manual for now)
    attacker.has_substitute = false;
    attacker.substitute_hp = 0;

    // Create second substitute (should succeed)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx2);
    TEST_ASSERT(!ctx2.move_failed, "Should succeed after substitute breaks");
    TEST_ASSERT(attacker.has_substitute, "Second substitute created");
}

// ============================================================================
// HP VALUES
// ============================================================================

TEST_CASE(Effect_Substitute_100MaxHP) {
    auto attacker = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                      domain::Type::Poison, 100, 50, 50, 65, 65, 45);
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.substitute_hp == 25, "100 / 4 = 25 substitute HP");
    TEST_ASSERT(attacker.current_hp == 75, "100 - 25 = 75 HP remaining");
}

TEST_CASE(Effect_Substitute_OddMaxHP) {
    auto attacker = CreatePikachu();  // 35 HP
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    // 35 / 4 = 8.75 → 8
    TEST_ASSERT(attacker.substitute_hp == 8, "35 / 4 rounds down to 8");
    TEST_ASSERT(attacker.current_hp == 27, "35 - 8 = 27 HP remaining");
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

TEST_CASE(Effect_Substitute_SetsAllFlags) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(attacker.has_substitute, "has_substitute flag should be set");
    TEST_ASSERT(attacker.substitute_hp > 0, "substitute_hp should be > 0");
    TEST_ASSERT(!ctx.move_failed, "move_failed should be false");
}

TEST_CASE(Effect_Substitute_NoChangesOnFailure) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSubstitute();
    attacker.current_hp = 10;  // Too low (need > 11)
    uint16_t original_hp = attacker.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Substitute(ctx);

    TEST_ASSERT(ctx.move_failed, "Move should fail");
    TEST_ASSERT(!attacker.has_substitute, "No substitute created");
    TEST_ASSERT(attacker.substitute_hp == 0, "Substitute HP should be 0");
    TEST_ASSERT(attacker.current_hp == original_hp, "HP should be unchanged");
}
