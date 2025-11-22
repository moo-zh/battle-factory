/**
 * @file test/host/damage/test_basic_damage.cpp
 * @brief Basic damage calculation tests
 *
 * Tests the fundamental damage calculation system using simple attacking moves
 * like Tackle and Ember.
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

/**
 * @brief Test fixture for basic damage tests
 *
 * Sets up common Pokemon for damage testing
 */
class BasicDamageTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // Initialize RNG with known seed for deterministic tests
        battle::random::Initialize(42);

        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

/**
 * @brief Verify that Tackle deals damage
 *
 * This is a smoke test to ensure the basic damage calculation works
 * and that our test helpers are set up correctly.
 */
TEST_F(BasicDamageTest, TackleDealsDamage) {
    uint16_t initial_hp = defender.current_hp;

    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_LT(defender.current_hp, initial_hp) << "Tackle should deal damage to defender";
    EXPECT_GT(ctx.damage_dealt, 0) << "Context should record damage dealt";
}

/**
 * @brief Verify that Ember deals damage
 *
 * Tests that a different move with different power also works correctly.
 */
TEST_F(BasicDamageTest, EmberDealsDamage) {
    uint16_t initial_hp = defender.current_hp;

    domain::MoveData ember = CreateEmber();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &ember);

    battle::effects::Effect_Hit(ctx);

    EXPECT_LT(defender.current_hp, initial_hp) << "Ember should deal damage to defender";
    EXPECT_GT(ctx.damage_dealt, 0) << "Context should record damage dealt";
}

/**
 * @brief Verify that stronger moves deal more damage
 *
 * Ember (40 power) should deal more damage than Tackle (35 power).
 */
TEST_F(BasicDamageTest, StrongerMoveDealsMoreDamage) {
    // Test Tackle damage
    battle::state::Pokemon defender1 = CreateBulbasaur();
    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender1, &tackle);
    battle::effects::Effect_Hit(ctx1);
    uint16_t tackle_damage = ctx1.damage_dealt;

    // Test Ember damage (reset RNG for fair comparison)
    battle::random::Initialize(42);
    battle::state::Pokemon defender2 = CreateBulbasaur();
    domain::MoveData ember = CreateEmber();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender2, &ember);
    battle::effects::Effect_Hit(ctx2);
    uint16_t ember_damage = ctx2.damage_dealt;

    EXPECT_GT(ember_damage, tackle_damage)
        << "Ember (40 power) should deal more damage than Tackle (35 power)";
}

/**
 * @brief Verify damage is deterministic with same seed
 *
 * Same seed should produce same damage roll.
 */
TEST_F(BasicDamageTest, DamageIsDeterministicWithSeed) {
    // First calculation
    battle::random::Initialize(100);
    battle::state::Pokemon defender1 = CreateBulbasaur();
    domain::MoveData tackle1 = CreateTackle();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender1, &tackle1);
    battle::effects::Effect_Hit(ctx1);
    uint16_t damage1 = ctx1.damage_dealt;

    // Second calculation with same seed
    battle::random::Initialize(100);
    battle::state::Pokemon defender2 = CreateBulbasaur();
    domain::MoveData tackle2 = CreateTackle();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender2, &tackle2);
    battle::effects::Effect_Hit(ctx2);
    uint16_t damage2 = ctx2.damage_dealt;

    EXPECT_EQ(damage1, damage2) << "Same seed should produce identical damage";
}

/**
 * @brief Verify that Pokemon with higher defense takes less damage
 */
TEST_F(BasicDamageTest, HigherDefenseReducesDamage) {
    // Low defense Pokemon
    battle::state::Pokemon low_def = CreatePokemonWithStats(50, 30, 50);
    domain::MoveData tackle1 = CreateTackle();
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &low_def, &tackle1);
    battle::effects::Effect_Hit(ctx1);
    uint16_t damage_to_low_def = ctx1.damage_dealt;

    // High defense Pokemon (reset RNG for fair comparison)
    battle::random::Initialize(42);
    battle::state::Pokemon high_def = CreatePokemonWithStats(50, 80, 50);
    domain::MoveData tackle2 = CreateTackle();
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &high_def, &tackle2);
    battle::effects::Effect_Hit(ctx2);
    uint16_t damage_to_high_def = ctx2.damage_dealt;

    EXPECT_LT(damage_to_high_def, damage_to_low_def)
        << "Higher defense should result in less damage taken";
}

// ============================================================================
// Tests migrated from test/EZ80/archived/ti84ce/foundation/test_hit.cpp
// ============================================================================

/**
 * @brief Verify basic damage calculation
 *
 * Migrated from: Effect_Hit_BasicDamage
 * Ensures defender takes damage and context records it correctly
 */
TEST_F(BasicDamageTest, BasicDamageCalculation) {
    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_LT(defender.current_hp, defender.max_hp) << "Defender HP should decrease";
    EXPECT_GT(ctx.damage_dealt, 0) << "Damage should be calculated";
    EXPECT_GT(ctx.damage_dealt, 10) << "Damage should be reasonable (> 10)";
    EXPECT_LT(ctx.damage_dealt, 30) << "Damage should be reasonable (< 30)";
}

/**
 * @brief Verify damage scales with Attack stat
 *
 * Migrated from: Effect_Hit_DamageScalesWithAttack
 * Higher Attack should deal more damage
 */
TEST_F(BasicDamageTest, DamageScalesWithAttack) {
    battle::state::Pokemon weak_attacker = CreatePokemonWithStats(30, 40, 50, 100);
    battle::state::Pokemon strong_attacker = CreatePokemonWithStats(90, 40, 50, 100);
    battle::state::Pokemon defender1 = CreateBulbasaur();
    battle::state::Pokemon defender2 = CreateBulbasaur();

    domain::MoveData tackle = CreateTackle();

    // Test weak attacker
    battle::BattleContext ctx1 = CreateBattleContext(&weak_attacker, &defender1, &tackle);
    battle::effects::Effect_Hit(ctx1);

    // Test strong attacker (reset RNG for fair comparison)
    battle::random::Initialize(42);
    battle::BattleContext ctx2 = CreateBattleContext(&strong_attacker, &defender2, &tackle);
    battle::effects::Effect_Hit(ctx2);

    EXPECT_GT(ctx2.damage_dealt, ctx1.damage_dealt) << "Higher Attack should deal more damage";
}

/**
 * @brief Verify damage scales with Defense stat
 *
 * Migrated from: Effect_Hit_DamageScalesWithDefense
 * Higher Defense should reduce damage taken
 */
TEST_F(BasicDamageTest, DamageScalesWithDefense) {
    battle::state::Pokemon attacker1 = CreateCharmander();
    battle::state::Pokemon attacker2 = CreateCharmander();
    battle::state::Pokemon weak_defender = CreatePokemonWithStats(50, 20, 50, 100);
    battle::state::Pokemon strong_defender = CreatePokemonWithStats(50, 80, 50, 100);

    domain::MoveData tackle = CreateTackle();

    // Test weak defender
    battle::BattleContext ctx1 = CreateBattleContext(&attacker1, &weak_defender, &tackle);
    battle::effects::Effect_Hit(ctx1);

    // Test strong defender (reset RNG for fair comparison)
    battle::random::Initialize(42);
    battle::BattleContext ctx2 = CreateBattleContext(&attacker2, &strong_defender, &tackle);
    battle::effects::Effect_Hit(ctx2);

    EXPECT_GT(ctx1.damage_dealt, ctx2.damage_dealt) << "Higher Defense should reduce damage taken";
}

/**
 * @brief Verify move can cause KO
 *
 * Migrated from: Effect_Hit_CanKo
 * Pokemon at 1 HP should faint
 */
TEST_F(BasicDamageTest, CanCauseKO) {
    battle::state::Pokemon weak_defender = CreatePokemonWithStats(50, 50, 50, 100);
    weak_defender.current_hp = 1;
    weak_defender.is_fainted = false;

    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &weak_defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_EQ(weak_defender.current_hp, 0) << "HP should be 0 after KO";
    EXPECT_TRUE(weak_defender.is_fainted) << "Faint flag should be set";
}

/**
 * @brief Verify minimum damage is dealt
 *
 * Migrated from: Effect_Hit_MinimumDamage
 * Even with very weak Attack vs. very high Defense, at least 1 damage
 */
TEST_F(BasicDamageTest, MinimumDamage) {
    battle::state::Pokemon weak_attacker = CreatePokemonWithStats(5, 50, 50, 100);
    battle::state::Pokemon tank_defender = CreatePokemonWithStats(50, 200, 50, 100);

    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&weak_attacker, &tank_defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_GE(ctx.damage_dealt, 1) << "Minimum damage should be 1 (Gen III rule)";
}

/**
 * @brief Verify HP is clamped at zero
 *
 * Migrated from: Effect_Hit_HpClampedAtZero
 * Overkill damage should not cause negative HP
 */
TEST_F(BasicDamageTest, HpClampedAtZero) {
    battle::state::Pokemon strong_attacker = CreatePokemonWithStats(200, 50, 50, 100);
    battle::state::Pokemon weak_defender = CreatePokemonWithStats(50, 50, 50, 10);

    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&strong_attacker, &weak_defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_EQ(weak_defender.current_hp, 0) << "HP should be clamped at 0";
    EXPECT_TRUE(weak_defender.is_fainted) << "Pokemon should be fainted";
}

/**
 * @brief Verify attacker is not modified
 *
 * Migrated from: Effect_Hit_DoesNotModifyAttacker
 * Basic damage moves should not affect the attacker
 */
TEST_F(BasicDamageTest, DoesNotModifyAttacker) {
    uint16_t original_hp = attacker.current_hp;
    bool original_fainted = attacker.is_fainted;

    domain::MoveData tackle = CreateTackle();
    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &tackle);

    battle::effects::Effect_Hit(ctx);

    EXPECT_EQ(attacker.current_hp, original_hp) << "Attacker HP should not change";
    EXPECT_EQ(attacker.is_fainted, original_fainted) << "Attacker faint state should not change";
}
