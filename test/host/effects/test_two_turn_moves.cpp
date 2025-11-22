/**
 * @file test/host/effects/test_two_turn_moves.cpp
 * @brief Tests for two-turn move effects (Solar Beam, Fly)
 *
 * Migrated from archived TI-84 CE tests:
 * - test/EZ80/archived/ti84ce/foundation/test_solar_beam.cpp (12 tests)
 * - test/EZ80/archived/ti84ce/foundation/test_fly.cpp (15 tests)
 *
 * This file tests moves that require charging on turn 1 and execute on turn 2.
 * Fly additionally has semi-invulnerable mechanics (OnAir state).
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace domain;  // For STAT_ constants

// ============================================================================
// SOLAR BEAM TESTS
// ============================================================================

/**
 * @brief Test fixture for Solar Beam tests
 */
class SolarBeamTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateBulbasaur();
        defender = CreateCharmander();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(SolarBeamTest, Turn1_StartsCharging) {
    domain::MoveData move = CreateSolarBeam();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx);

    EXPECT_TRUE(attacker.is_charging) << "Solar Beam should set is_charging on Turn 1";
    EXPECT_EQ(attacker.charging_move, Move::SolarBeam)
        << "charging_move should be set to SolarBeam";
    EXPECT_FALSE(ctx.move_failed) << "Move should succeed on Turn 1";
}

TEST_F(SolarBeamTest, Turn1_NoDamage) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateSolarBeam();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx);

    EXPECT_EQ(defender.current_hp, original_hp) << "No damage should be dealt on charging turn";
    EXPECT_EQ(ctx.damage_dealt, 0) << "damage_dealt should be 0 on Turn 1";
}

TEST_F(SolarBeamTest, Turn2_ExecutesAttack) {
    domain::MoveData move = CreateSolarBeam();

    // Simulate Turn 1: Start charging
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Simulate Turn 2: Execute attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    EXPECT_FALSE(attacker.is_charging) << "is_charging should be cleared after attack";
    EXPECT_GT(ctx2.damage_dealt, 0) << "Damage should be dealt on Turn 2";
    EXPECT_LT(defender.current_hp, defender.max_hp) << "Defender should take damage";
}

TEST_F(SolarBeamTest, Turn2_ClearsChargingFlag) {
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);
    EXPECT_TRUE(attacker.is_charging) << "Should be charging after Turn 1";

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);
    EXPECT_FALSE(attacker.is_charging) << "Should not be charging after Turn 2";
}

TEST_F(SolarBeamTest, HighPower) {
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // Solar Beam has 120 power, should deal significant damage
    EXPECT_GE(ctx2.damage_dealt, 5) << "Solar Beam should deal at least 5 damage";
}

TEST_F(SolarBeamTest, AccuracyCheckOnTurn2) {
    domain::MoveData move = CreateSolarBeam();
    move.accuracy = 100;  // Ensure it hits for this test

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Attack (accuracy check happens here)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // With 100% accuracy, should always hit
    EXPECT_FALSE(ctx2.move_failed) << "Solar Beam should hit with 100% accuracy";
    EXPECT_GT(ctx2.damage_dealt, 0) << "Damage should be dealt when move hits";
}

TEST_F(SolarBeamTest, MissAfterCharging) {
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Simulate miss by setting move_failed before damage calc
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    ctx2.move_failed = true;  // Force miss
    battle::effects::Effect_SolarBeam(ctx2);

    // Even if missed, charging should be cleared
    EXPECT_FALSE(attacker.is_charging) << "Charging should be cleared even on miss";
}

TEST_F(SolarBeamTest, ProtectionBlocksAttack) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Defender is protected
    defender.is_protected = true;
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // Protection should block the attack
    EXPECT_EQ(defender.current_hp, original_hp) << "Protection should block Solar Beam";
    EXPECT_FALSE(attacker.is_charging) << "Charging should be cleared even when blocked";
}

TEST_F(SolarBeamTest, StatStagesApply) {
    domain::MoveData move = CreateSolarBeam();

    // Raise attacker's Sp. Attack by 2 stages
    attacker.stat_stages[STAT_SPATK] = +2;

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Attack with boosted Sp. Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // Damage should be higher due to stat boost
    EXPECT_GE(ctx2.damage_dealt, 8) << "Boosted Sp. Attack should increase damage";
}

TEST_F(SolarBeamTest, DefenderFaints) {
    defender.current_hp = 5;  // Set defender to low HP
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Attack (should KO)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // Defender should faint
    EXPECT_EQ(defender.current_hp, 0) << "Defender should faint from Solar Beam";
    EXPECT_TRUE(defender.is_fainted) << "Defender should be marked as fainted";
}

TEST_F(SolarBeamTest, NoSelfDamage) {
    uint16_t original_hp = attacker.current_hp;
    domain::MoveData move = CreateSolarBeam();

    // Turn 1: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);

    // Attacker should not take damage
    EXPECT_EQ(attacker.current_hp, original_hp) << "Solar Beam should not damage user";
}

TEST_F(SolarBeamTest, MultipleChargesSequential) {
    domain::MoveData move = CreateSolarBeam();

    // First Solar Beam: Charge
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx1);
    EXPECT_TRUE(attacker.is_charging) << "First charge should set flag";

    // First Solar Beam: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx2);
    EXPECT_FALSE(attacker.is_charging) << "First attack should clear flag";

    // Second Solar Beam: Charge again
    battle::BattleContext ctx3 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx3);
    EXPECT_TRUE(attacker.is_charging) << "Second charge should set flag again";

    // Second Solar Beam: Attack
    battle::BattleContext ctx4 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_SolarBeam(ctx4);
    EXPECT_FALSE(attacker.is_charging) << "Second attack should clear flag";
}

// ============================================================================
// FLY TESTS
// ============================================================================

/**
 * @brief Test fixture for Fly tests
 */
class FlyTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreatePidgey();
        defender = CreateCharmander();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

TEST_F(FlyTest, Turn1_StartsCharging) {
    domain::MoveData move = CreateFly();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx);

    EXPECT_TRUE(attacker.is_charging) << "Fly should set is_charging on Turn 1";
    EXPECT_EQ(attacker.charging_move, Move::Fly) << "charging_move should be set to Fly";
    EXPECT_FALSE(ctx.move_failed) << "Move should succeed on Turn 1";
}

TEST_F(FlyTest, Turn1_NoDamage) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateFly();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx);

    EXPECT_EQ(defender.current_hp, original_hp) << "No damage should be dealt on fly-up turn";
    EXPECT_EQ(ctx.damage_dealt, 0) << "damage_dealt should be 0 on Turn 1";
}

TEST_F(FlyTest, Turn2_ExecutesAttack) {
    domain::MoveData move = CreateFly();

    // Simulate Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Simulate Turn 2: Attack from air
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    EXPECT_FALSE(attacker.is_charging) << "is_charging should be cleared after attack";
    EXPECT_GT(ctx2.damage_dealt, 0) << "Damage should be dealt on Turn 2";
    EXPECT_LT(defender.current_hp, defender.max_hp) << "Defender should take damage";
}

TEST_F(FlyTest, Turn2_ClearsChargingFlag) {
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);
    EXPECT_TRUE(attacker.is_charging) << "Should be charging after Turn 1";

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);
    EXPECT_FALSE(attacker.is_charging) << "Should not be charging after Turn 2";
}

TEST_F(FlyTest, Turn1_SetsSemiInvulnerableFlag) {
    domain::MoveData move = CreateFly();

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx);

    EXPECT_TRUE(attacker.is_semi_invulnerable) << "Fly should set is_semi_invulnerable on Turn 1";
    EXPECT_EQ(attacker.semi_invulnerable_type, battle::state::SemiInvulnerableType::OnAir)
        << "semi_invulnerable_type should be OnAir";
}

TEST_F(FlyTest, Turn2_ClearsSemiInvulnerableFlag) {
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up (become semi-invulnerable)
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);
    EXPECT_TRUE(attacker.is_semi_invulnerable) << "Should be semi-invulnerable after Turn 1";

    // Turn 2: Attack (clear semi-invulnerable)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);
    EXPECT_FALSE(attacker.is_semi_invulnerable) << "Should not be semi-invulnerable after Turn 2";
    EXPECT_EQ(attacker.semi_invulnerable_type, battle::state::SemiInvulnerableType::None)
        << "semi_invulnerable_type should be None after attack";
}

TEST_F(FlyTest, SemiInvulnerableTypeIsOnAir) {
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Verify the semi-invulnerable type is specifically OnAir
    EXPECT_EQ(attacker.semi_invulnerable_type, battle::state::SemiInvulnerableType::OnAir)
        << "Fly should set semi_invulnerable_type to OnAir";
    EXPECT_NE(attacker.semi_invulnerable_type, battle::state::SemiInvulnerableType::Underground)
        << "Fly should not set Underground type";
    EXPECT_NE(attacker.semi_invulnerable_type, battle::state::SemiInvulnerableType::Underwater)
        << "Fly should not set Underwater type";
}

TEST_F(FlyTest, AccuracyCheckOnTurn2) {
    domain::MoveData move = CreateFly();
    move.accuracy = 100;  // Ensure it hits for this test

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Attack (accuracy check happens here)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // With 100% accuracy, should always hit
    EXPECT_FALSE(ctx2.move_failed) << "Fly should hit with 100% accuracy";
    EXPECT_GT(ctx2.damage_dealt, 0) << "Damage should be dealt when move hits";
}

TEST_F(FlyTest, MissAfterFlying) {
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Simulate miss by setting move_failed before damage calc
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    ctx2.move_failed = true;  // Force miss
    battle::effects::Effect_Fly(ctx2);

    // Even if missed, charging and semi-invulnerable should be cleared
    EXPECT_FALSE(attacker.is_charging) << "Charging should be cleared even on miss";
    EXPECT_FALSE(attacker.is_semi_invulnerable)
        << "Semi-invulnerable should be cleared even on miss";
}

TEST_F(FlyTest, ProtectionBlocksAttack) {
    uint16_t original_hp = defender.current_hp;
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Defender is protected
    defender.is_protected = true;
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // Protection should block the attack
    EXPECT_EQ(defender.current_hp, original_hp) << "Protection should block Fly";
    EXPECT_FALSE(attacker.is_charging) << "Charging should be cleared even when blocked";
    EXPECT_FALSE(attacker.is_semi_invulnerable)
        << "Semi-invulnerable should be cleared even when blocked";
}

TEST_F(FlyTest, DecentPower) {
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // Fly has 70 power, should deal decent damage
    EXPECT_GE(ctx2.damage_dealt, 3) << "Fly should deal at least 3 damage";
}

TEST_F(FlyTest, StatStagesApply) {
    domain::MoveData move = CreateFly();

    // Raise attacker's Attack by 2 stages
    attacker.stat_stages[STAT_ATK] = +2;

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Attack with boosted Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // Damage should be higher due to stat boost
    EXPECT_GE(ctx2.damage_dealt, 5) << "Boosted Attack should increase damage";
}

TEST_F(FlyTest, DefenderFaints) {
    defender.current_hp = 3;  // Set defender to low HP
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Attack (should KO)
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // Defender should faint
    EXPECT_EQ(defender.current_hp, 0) << "Defender should faint from Fly";
    EXPECT_TRUE(defender.is_fainted) << "Defender should be marked as fainted";
}

TEST_F(FlyTest, NoSelfDamage) {
    uint16_t original_hp = attacker.current_hp;
    domain::MoveData move = CreateFly();

    // Turn 1: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);

    // Turn 2: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);

    // Attacker should not take damage
    EXPECT_EQ(attacker.current_hp, original_hp) << "Fly should not damage user";
}

TEST_F(FlyTest, MultipleUsesSequential) {
    domain::MoveData move = CreateFly();

    // First Fly: Fly up
    battle::BattleContext ctx1 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx1);
    EXPECT_TRUE(attacker.is_charging) << "First fly-up should set flag";
    EXPECT_TRUE(attacker.is_semi_invulnerable) << "First fly-up should be semi-invulnerable";

    // First Fly: Attack
    battle::BattleContext ctx2 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx2);
    EXPECT_FALSE(attacker.is_charging) << "First attack should clear charging flag";
    EXPECT_FALSE(attacker.is_semi_invulnerable) << "First attack should clear semi-invulnerable";

    // Second Fly: Fly up again
    battle::BattleContext ctx3 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx3);
    EXPECT_TRUE(attacker.is_charging) << "Second fly-up should set flag again";
    EXPECT_TRUE(attacker.is_semi_invulnerable) << "Second fly-up should be semi-invulnerable";

    // Second Fly: Attack
    battle::BattleContext ctx4 = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Fly(ctx4);
    EXPECT_FALSE(attacker.is_charging) << "Second attack should clear charging flag";
    EXPECT_FALSE(attacker.is_semi_invulnerable) << "Second attack should clear semi-invulnerable";
}
