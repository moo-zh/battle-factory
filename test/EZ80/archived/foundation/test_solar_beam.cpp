/**
 * @file test/unit/effects/test_solar_beam.cpp
 * @brief Tests for Effect_SolarBeam (Solar Beam - two-turn move)
 */

#include "../../../src/battle/effects/basic.hpp"
#include "../../../src/battle/state/pokemon.hpp"
#include "../../../src/domain/move.hpp"
#include "../../../src/domain/species.hpp"
#include "../../../src/domain/stats.hpp"
#include "framework.hpp"

// Include common test helpers
#include "../test_helpers.hpp"

TEST_CASE(Effect_SolarBeam_Turn1_StartsCharging) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx);

    TEST_ASSERT(attacker.is_charging, "Solar Beam should set is_charging on Turn 1");
    TEST_ASSERT(attacker.charging_move == domain::Move::SolarBeam,
                "charging_move should be set to SolarBeam");
    TEST_ASSERT(!ctx.move_failed, "Move should succeed on Turn 1");
}

TEST_CASE(Effect_SolarBeam_Turn1_NoDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "No damage should be dealt on charging turn");
    TEST_ASSERT(ctx.damage_dealt == 0, "damage_dealt should be 0 on Turn 1");
}

TEST_CASE(Effect_SolarBeam_Turn2_ExecutesAttack) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Simulate Turn 1: Start charging
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Simulate Turn 2: Execute attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    TEST_ASSERT(!attacker.is_charging, "is_charging should be cleared after attack");
    TEST_ASSERT(ctx2.damage_dealt > 0, "Damage should be dealt on Turn 2");
    TEST_ASSERT(defender.current_hp < defender.max_hp, "Defender should take damage");
}

TEST_CASE(Effect_SolarBeam_Turn2_ClearsChargingFlag) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);
    TEST_ASSERT(attacker.is_charging, "Should be charging after Turn 1");

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);
    TEST_ASSERT(!attacker.is_charging, "Should not be charging after Turn 2");
}

TEST_CASE(Effect_SolarBeam_HighPower) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // Solar Beam has 120 power, should deal significant damage
    // Grass-type vs Fire-type (not very effective, 0.5x)
    // Base stats: Bulbasaur SpA 65, Charmander SpD 50
    // Expect moderate damage despite resistance
    TEST_ASSERT(ctx2.damage_dealt >= 5, "Solar Beam should deal at least 5 damage");
}

TEST_CASE(Effect_SolarBeam_AccuracyCheckOnTurn2) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();
    move.accuracy = 100;  // Ensure it hits for this test

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Attack (accuracy check happens here)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // With 100% accuracy, should always hit
    TEST_ASSERT(!ctx2.move_failed, "Solar Beam should hit with 100% accuracy");
    TEST_ASSERT(ctx2.damage_dealt > 0, "Damage should be dealt when move hits");
}

TEST_CASE(Effect_SolarBeam_MissAfterCharging) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Simulate miss by setting move_failed before damage calc
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    ctx2.move_failed = true;  // Force miss
    Effect_SolarBeam(ctx2);

    // Even if missed, charging should be cleared
    TEST_ASSERT(!attacker.is_charging, "Charging should be cleared even on miss");
}

TEST_CASE(Effect_SolarBeam_ProtectionBlocksAttack) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();
    uint16_t original_hp = defender.current_hp;

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Defender is protected
    defender.is_protected = true;
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // Protection should block the attack
    TEST_ASSERT(defender.current_hp == original_hp, "Protection should block Solar Beam");
    TEST_ASSERT(!attacker.is_charging, "Charging should be cleared even when blocked");
}

// NOTE: Critical hits and type effectiveness are tested in CalculateDamage tests
// Solar Beam just calls CalculateDamage, so we don't need to re-test those mechanics here

TEST_CASE(Effect_SolarBeam_StatStagesApply) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Raise attacker's Sp. Attack by 2 stages
    attacker.stat_stages[STAT_SPATK] = +2;

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Attack with boosted Sp. Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // Damage should be higher due to stat boost
    TEST_ASSERT(ctx2.damage_dealt >= 8, "Boosted Sp. Attack should increase damage");
}

TEST_CASE(Effect_SolarBeam_DefenderFaints) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // Set defender to low HP
    defender.current_hp = 5;

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Attack (should KO)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // Defender should faint
    TEST_ASSERT(defender.current_hp == 0, "Defender should faint from Solar Beam");
    TEST_ASSERT(defender.is_fainted, "Defender should be marked as fainted");
}

TEST_CASE(Effect_SolarBeam_NoSelfDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();
    uint16_t original_hp = attacker.current_hp;

    // Turn 1: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);

    // Attacker should not take damage
    TEST_ASSERT(attacker.current_hp == original_hp, "Solar Beam should not damage user");
}

TEST_CASE(Effect_SolarBeam_MultipleChargesSequential) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateSolarBeam();

    // First Solar Beam: Charge
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx1);
    TEST_ASSERT(attacker.is_charging, "First charge should set flag");

    // First Solar Beam: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx2);
    TEST_ASSERT(!attacker.is_charging, "First attack should clear flag");

    // Second Solar Beam: Charge again
    BattleContext ctx3 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx3);
    TEST_ASSERT(attacker.is_charging, "Second charge should set flag again");

    // Second Solar Beam: Attack
    BattleContext ctx4 = SetupContext(&attacker, &defender, &move);
    Effect_SolarBeam(ctx4);
    TEST_ASSERT(!attacker.is_charging, "Second attack should clear flag");
}
