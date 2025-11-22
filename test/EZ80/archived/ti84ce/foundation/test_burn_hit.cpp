/**
 * @file test_effect_burn_hit.cpp
 * @brief Tests for Effect_BurnHit (Ember, Flamethrower, etc.)
 *
 * These tests verify that damaging moves with a secondary burn effect work correctly.
 * This builds upon Effect_Hit by adding status condition logic.
 */

#include "framework.hpp"

// Include real implementation headers
#include "../../../src/battle/context.hpp"
#include "../../../src/battle/effects/basic.hpp"
#include "../../../src/battle/state/pokemon.hpp"
#include "../../../src/domain/move.hpp"
#include "../../../src/domain/species.hpp"
// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../src/battle/effects/basic.hpp"

// ============================================================================

TEST_CASE(Effect_BurnHit_DealsDamage) {
    // Setup: Charmander uses Ember on Bulbasaur
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateEmber();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_BurnHit(ctx);

    // Assert: Defender should take damage
    TEST_ASSERT(defender.current_hp < defender.max_hp, "Defender HP should decrease");
    TEST_ASSERT(ctx.damage_dealt > 0, "Damage should be calculated");
}

TEST_CASE(Effect_BurnHit_CanApplyBurn) {
    // Setup: Test probabilistically by running many trials
    int burns = 0;
    const int trials = 100;

    for (int i = 0; i < trials; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0) {  // STATUS1_BURN is non-zero
            burns++;
        }
    }

    // With 10% burn chance, expect around 10 burns out of 100
    // Allow 3-20 range for statistical variance
    TEST_ASSERT(burns >= 3, "Should have some burns (at least 3/100)");
    TEST_ASSERT(burns <= 20, "Should not burn too often (max 20/100)");
}

TEST_CASE(Effect_BurnHit_DamageAndBurnBothApply) {
    // Setup: Verify both damage and burn can occur in same attack
    // Run multiple trials to find case where burn applies
    bool found_burn_with_damage = false;
    for (int i = 0; i < 200 && !found_burn_with_damage; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0 && test_defender.current_hp < test_defender.max_hp) {
            found_burn_with_damage = true;
        }
    }

    TEST_ASSERT(found_burn_with_damage, "Both damage and burn should apply in same attack");
}

// ============================================================================
// BURN IMMUNITY TESTS
// ============================================================================

TEST_CASE(Effect_BurnHit_FireTypeImmuneToBurn) {
    // Setup: Fire-type defender should be immune to burn
    // Run many trials - Fire type should NEVER burn
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateCharmander();  // Fire type
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        TEST_ASSERT_BATCH(test_defender.status1 == 0, "Fire type immune to burn", 100);
    }
}

TEST_CASE(Effect_BurnHit_AlreadyStatusedCantBurn) {
    // Setup: Pokemon with existing status cannot be burned
    // Run many trials - already statused Pokemon should NEVER burn
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        test_defender.status1 = 1;  // Pre-existing status
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        TEST_ASSERT_BATCH(test_defender.status1 == 1, "Already-statused Pokemon cannot burn", 100);
    }
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE(Effect_BurnHit_FaintedTargetNotBurned) {
    // Setup: Pokemon that faints from damage should not be burned
    // Run many trials - dead Pokemon should never burn
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        test_defender.current_hp = 1;  // Will die
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        TEST_ASSERT_BATCH(test_defender.current_hp == 0, "Fainted Pokemon HP is 0", 100);
        TEST_ASSERT_BATCH(test_defender.status1 == 0, "Fainted Pokemon not burned", 100);
        TEST_ASSERT_BATCH(test_defender.is_fainted == true, "Faint flag set correctly", 100);
    }
}

TEST_CASE(Effect_BurnHit_DoesNotModifyAttacker) {
    // Setup: Verify attacker is not damaged or affected
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateEmber();

    uint16_t original_hp = attacker.current_hp;
    uint8_t original_status = attacker.status1;

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_BurnHit(ctx);

    // Assert: Attacker should be unchanged
    TEST_ASSERT_EQ(attacker.current_hp, original_hp, "Attacker HP should not change");
    TEST_ASSERT_EQ(attacker.status1, original_status, "Attacker status should not change");
    TEST_ASSERT(!attacker.is_fainted, "Attacker should not faint");
}

TEST_CASE(Effect_BurnHit_ZeroPowerMoveStillChecksBurn) {
    // Edge case: If move somehow has 0 power, burn should still be checked
    // This is hypothetical but tests command separation
    int burns = 0;
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        auto test_move = CreateEmber();
        test_move.power = 0;
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        if (test_defender.status1 != 0) {
            burns++;
        }
    }

    // Should still have ~10% burn rate even with 0 damage
    TEST_ASSERT(burns >= 3, "Zero-damage move should still roll for burn");
}

// ============================================================================
// FUTURE TESTS (commented out until features implemented)
// ============================================================================

/*
TEST_CASE(Effect_BurnHit_WaterVeilImmune) {
    // TODO: Enable when abilities are implemented
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    defender.ability = Ability::WaterVeil;
    auto move = CreateEmber();

    // Water Veil should prevent burn
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreateCharmander();
        auto test_defender = CreateBulbasaur();
        test_defender.ability = Ability::WaterVeil;
        auto test_move = CreateEmber();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_BurnHit(test_ctx);

        TEST_ASSERT_EQ(test_defender.status1, 0, "Water Veil should prevent burn");
    }
}

TEST_CASE(Effect_BurnHit_SafeguardPreventsBurn) {
    // TODO: Enable when field effects are implemented
    // Safeguard should prevent burn application
}

TEST_CASE(Effect_BurnHit_SubstituteAllowsBurn) {
    // TODO: Enable when Substitute is implemented
    // Gen III: Substitute blocks damage but NOT secondary status
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    defender.status2 |= STATUS2_SUBSTITUTE;
    defender.substitute_hp = 50;
    auto move = CreateEmber();

    // Damage should hit substitute, but burn can still apply
}

TEST_CASE(Effect_BurnHit_MissDoesNotBurn) {
    // TODO: Enable when accuracy checks are fully implemented
    // When move misses, no damage and no burn
}
*/
