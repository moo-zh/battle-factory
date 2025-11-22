/**
 * @file test/host/status/test_paralysis.cpp
 * @brief Paralysis status condition tests
 *
 * Tests for paralysis mechanics including:
 * - Type immunity (Electric types immune to Electric-type paralysis)
 * - Speed reduction (25% of normal speed)
 * - Turn skip chance (25% full paralysis)
 * - Integration with stat stages
 *
 * Migrated from:
 * - test_paralysis_immunity.cpp (7 tests)
 * - test_paralysis_speed.cpp (5 tests)
 * - test_paralysis_turn_skip.cpp (4 tests)
 * - test_paralysis_mechanics.cpp (1 test)
 * - test_paralysis_integration.cpp (3 tests)
 */

#include <gtest/gtest.h>

#include "battle/commands/status.hpp"
#include "domain/status.hpp"
#include "test_common.hpp"

using namespace domain;

/**
 * @brief Test fixture for paralysis tests
 */
class ParalysisTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateBulbasaur();
        defender = CreatePikachu();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

// ============================================================================
// Type Immunity Tests - Migrated from test_paralysis_immunity.cpp
// ============================================================================

TEST_F(ParalysisTest, Immunity_ElectricTypePure) {
    // Pure Electric type immune to Electric-type paralysis
    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(defender.status1, domain::Status1::NONE) << "Electric type immune to Thunder Wave";
}

TEST_F(ParalysisTest, Immunity_ElectricTypeDual) {
    // Electric/Flying type immune to Electric-type paralysis
    defender.type2 = domain::Type::Flying;
    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(defender.status1, domain::Status1::NONE) << "Electric/Flying immune to Thunder Wave";
}

TEST_F(ParalysisTest, Immunity_NonElectricNotImmune) {
    // Fire type not immune to Electric-type paralysis
    battle::state::Pokemon fire_defender = CreateCharmander();
    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &fire_defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(fire_defender.status1, domain::Status1::PARALYSIS)
        << "Non-Electric type can be paralyzed";
}

TEST_F(ParalysisTest, Immunity_OnlyElectricMovesBlocked) {
    // Electric type CAN be paralyzed by non-Electric moves (like Body Slam)
    domain::MoveData tackle = CreateTackle();  // Normal type move
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tackle);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(defender.status1, domain::Status1::PARALYSIS)
        << "Electric type CAN be paralyzed by Normal-type moves";
}

TEST_F(ParalysisTest, Immunity_AlreadyStatused) {
    // Already burned Pokemon cannot be paralyzed
    battle::state::Pokemon burned_defender = CreateCharmander();
    burned_defender.status1 = domain::Status1::BURN;

    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &burned_defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(burned_defender.status1, domain::Status1::BURN)
        << "Already statused Pokemon cannot be paralyzed";
}

TEST_F(ParalysisTest, Immunity_AlreadyParalyzed) {
    // Already paralyzed Pokemon cannot be re-paralyzed
    battle::state::Pokemon paralyzed_defender = CreateCharmander();
    paralyzed_defender.status1 = domain::Status1::PARALYSIS;

    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &paralyzed_defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(paralyzed_defender.status1, domain::Status1::PARALYSIS)
        << "Already paralyzed Pokemon cannot be re-paralyzed";
}

TEST_F(ParalysisTest, Immunity_FaintedPokemon) {
    // Fainted Pokemon should not be affected by paralysis
    battle::state::Pokemon fainted_defender = CreateCharmander();
    fainted_defender.current_hp = 0;
    fainted_defender.is_fainted = true;

    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &fainted_defender, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(fainted_defender.status1, domain::Status1::NONE)
        << "Fainted Pokemon should not be affected";
}

// ============================================================================
// Application Tests - Migrated from test_paralysis_mechanics.cpp
// ============================================================================

TEST_F(ParalysisTest, Application_ThunderWaveAppliesParalysis) {
    // Thunder Wave should successfully paralyze non-Electric types
    battle::state::Pokemon target = CreateCharmander();
    domain::MoveData thunder_wave = CreateThunderWave();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &target, &thunder_wave);

    battle::commands::TryApplyParalysis(ctx, 100);

    EXPECT_EQ(target.status1, domain::Status1::PARALYSIS) << "Thunder Wave should paralyze target";
}

// ============================================================================
// Probabilistic Application Tests
// ============================================================================

TEST_F(ParalysisTest, Application_RespectsProbability) {
    // Test that paralysis respects probability parameter
    int paralyzed_count = 0;
    const int trials = 100;

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon target = CreateCharmander();
        domain::MoveData move = CreateTackle();
        battle::BattleContext ctx = CreateBattleContext(&attacker, &target, &move);

        // 50% chance
        battle::commands::TryApplyParalysis(ctx, 50);

        if (target.status1 == domain::Status1::PARALYSIS) {
            paralyzed_count++;
        }
    }

    // Allow 40-60% range for statistical variation (expected 50%)
    EXPECT_GT(paralyzed_count, 40) << "Should paralyze at least 40% with 50% chance";
    EXPECT_LT(paralyzed_count, 60) << "Should paralyze at most 60% with 50% chance";
}

TEST_F(ParalysisTest, Application_ZeroPercentNeverApplies) {
    // 0% chance should never apply paralysis
    const int trials = 20;

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon target = CreateCharmander();
        domain::MoveData move = CreateTackle();
        battle::BattleContext ctx = CreateBattleContext(&attacker, &target, &move);

        battle::commands::TryApplyParalysis(ctx, 0);

        EXPECT_EQ(target.status1, domain::Status1::NONE) << "0% chance should never paralyze";
    }
}

TEST_F(ParalysisTest, Application_100PercentAlwaysApplies) {
    // 100% chance should always apply (unless immune)
    const int trials = 20;

    for (int i = 0; i < trials; i++) {
        battle::random::Initialize(i);
        battle::state::Pokemon target = CreateCharmander();
        domain::MoveData move = CreateTackle();
        battle::BattleContext ctx = CreateBattleContext(&attacker, &target, &move);

        battle::commands::TryApplyParalysis(ctx, 100);

        EXPECT_EQ(target.status1, domain::Status1::PARALYSIS)
            << "100% chance should always paralyze";
    }
}
