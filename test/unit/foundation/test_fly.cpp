/**
 * @file test/unit/effects/test_fly.cpp
 * @brief Tests for Effect_Fly (Fly - two-turn semi-invulnerable move)
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
// BASIC TWO-TURN BEHAVIOR
// ============================================================================

TEST_CASE(Effect_Fly_Turn1_StartsCharging) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx);

    TEST_ASSERT(attacker.is_charging, "Fly should set is_charging on Turn 1");
    TEST_ASSERT(attacker.charging_move == domain::Move::Fly, "charging_move should be set to Fly");
    TEST_ASSERT(!ctx.move_failed, "Move should succeed on Turn 1");
}

TEST_CASE(Effect_Fly_Turn1_NoDamage) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "No damage should be dealt on fly-up turn");
    TEST_ASSERT(ctx.damage_dealt == 0, "damage_dealt should be 0 on Turn 1");
}

TEST_CASE(Effect_Fly_Turn2_ExecutesAttack) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Simulate Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Simulate Turn 2: Attack from air
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    TEST_ASSERT(!attacker.is_charging, "is_charging should be cleared after attack");
    TEST_ASSERT(ctx2.damage_dealt > 0, "Damage should be dealt on Turn 2");
    TEST_ASSERT(defender.current_hp < defender.max_hp, "Defender should take damage");
}

TEST_CASE(Effect_Fly_Turn2_ClearsChargingFlag) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);
    TEST_ASSERT(attacker.is_charging, "Should be charging after Turn 1");

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);
    TEST_ASSERT(!attacker.is_charging, "Should not be charging after Turn 2");
}

// ============================================================================
// SEMI-INVULNERABLE MECHANICS
// ============================================================================

TEST_CASE(Effect_Fly_Turn1_SetsSemiInvulnerableFlag) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx);

    TEST_ASSERT(attacker.is_semi_invulnerable, "Fly should set is_semi_invulnerable on Turn 1");
    TEST_ASSERT(attacker.semi_invulnerable_type == battle::state::SemiInvulnerableType::OnAir,
                "semi_invulnerable_type should be OnAir");
}

TEST_CASE(Effect_Fly_Turn2_ClearsSemiInvulnerableFlag) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Turn 1: Fly up (become semi-invulnerable)
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);
    TEST_ASSERT(attacker.is_semi_invulnerable, "Should be semi-invulnerable after Turn 1");

    // Turn 2: Attack (clear semi-invulnerable)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);
    TEST_ASSERT(!attacker.is_semi_invulnerable, "Should not be semi-invulnerable after Turn 2");
    TEST_ASSERT(attacker.semi_invulnerable_type == battle::state::SemiInvulnerableType::None,
                "semi_invulnerable_type should be None after attack");
}

TEST_CASE(Effect_Fly_SemiInvulnerableTypeIsOnAir) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Verify the semi-invulnerable type is specifically OnAir (not Underground or Underwater)
    TEST_ASSERT(attacker.semi_invulnerable_type == battle::state::SemiInvulnerableType::OnAir,
                "Fly should set semi_invulnerable_type to OnAir");
    TEST_ASSERT(attacker.semi_invulnerable_type != battle::state::SemiInvulnerableType::Underground,
                "Fly should not set Underground type");
    TEST_ASSERT(attacker.semi_invulnerable_type != battle::state::SemiInvulnerableType::Underwater,
                "Fly should not set Underwater type");
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_CASE(Effect_Fly_AccuracyCheckOnTurn2) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();
    move.accuracy = 100;  // Ensure it hits for this test

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Attack (accuracy check happens here)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // With 100% accuracy, should always hit
    TEST_ASSERT(!ctx2.move_failed, "Fly should hit with 100% accuracy");
    TEST_ASSERT(ctx2.damage_dealt > 0, "Damage should be dealt when move hits");
}

TEST_CASE(Effect_Fly_MissAfterFlying) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Simulate miss by setting move_failed before damage calc
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    ctx2.move_failed = true;  // Force miss
    Effect_Fly(ctx2);

    // Even if missed, charging and semi-invulnerable should be cleared
    TEST_ASSERT(!attacker.is_charging, "Charging should be cleared even on miss");
    TEST_ASSERT(!attacker.is_semi_invulnerable, "Semi-invulnerable should be cleared even on miss");
}

TEST_CASE(Effect_Fly_ProtectionBlocksAttack) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();
    uint16_t original_hp = defender.current_hp;

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Defender is protected
    defender.is_protected = true;
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // Protection should block the attack
    TEST_ASSERT(defender.current_hp == original_hp, "Protection should block Fly");
    TEST_ASSERT(!attacker.is_charging, "Charging should be cleared even when blocked");
    TEST_ASSERT(!attacker.is_semi_invulnerable,
                "Semi-invulnerable should be cleared even when blocked");
}

// ============================================================================
// DAMAGE CALCULATIONS
// ============================================================================

TEST_CASE(Effect_Fly_DecentPower) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // Fly has 70 power, should deal decent damage
    // Base stats: Pidgey Atk 45, Charmander Def 43
    TEST_ASSERT(ctx2.damage_dealt >= 3, "Fly should deal at least 3 damage");
}

TEST_CASE(Effect_Fly_StatStagesApply) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Raise attacker's Attack by 2 stages
    attacker.stat_stages[STAT_ATK] = +2;

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Attack with boosted Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // Damage should be higher due to stat boost
    TEST_ASSERT(ctx2.damage_dealt >= 5, "Boosted Attack should increase damage");
}

// ============================================================================
// FAINTING
// ============================================================================

TEST_CASE(Effect_Fly_DefenderFaints) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // Set defender to low HP
    defender.current_hp = 3;

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Attack (should KO)
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // Defender should faint
    TEST_ASSERT(defender.current_hp == 0, "Defender should faint from Fly");
    TEST_ASSERT(defender.is_fainted, "Defender should be marked as fainted");
}

TEST_CASE(Effect_Fly_NoSelfDamage) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();
    uint16_t original_hp = attacker.current_hp;

    // Turn 1: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);

    // Turn 2: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);

    // Attacker should not take damage
    TEST_ASSERT(attacker.current_hp == original_hp, "Fly should not damage user");
}

// ============================================================================
// MULTIPLE USES
// ============================================================================

TEST_CASE(Effect_Fly_MultipleUsesSequential) {
    auto attacker = CreatePidgey();
    auto defender = CreateCharmander();
    auto move = CreateFly();

    // First Fly: Fly up
    BattleContext ctx1 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx1);
    TEST_ASSERT(attacker.is_charging, "First fly-up should set flag");
    TEST_ASSERT(attacker.is_semi_invulnerable, "First fly-up should be semi-invulnerable");

    // First Fly: Attack
    BattleContext ctx2 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx2);
    TEST_ASSERT(!attacker.is_charging, "First attack should clear charging flag");
    TEST_ASSERT(!attacker.is_semi_invulnerable, "First attack should clear semi-invulnerable");

    // Second Fly: Fly up again
    BattleContext ctx3 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx3);
    TEST_ASSERT(attacker.is_charging, "Second fly-up should set flag again");
    TEST_ASSERT(attacker.is_semi_invulnerable, "Second fly-up should be semi-invulnerable");

    // Second Fly: Attack
    BattleContext ctx4 = SetupContext(&attacker, &defender, &move);
    Effect_Fly(ctx4);
    TEST_ASSERT(!attacker.is_charging, "Second attack should clear charging flag");
    TEST_ASSERT(!attacker.is_semi_invulnerable, "Second attack should clear semi-invulnerable");
}
