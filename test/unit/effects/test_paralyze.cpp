/**
 * @file test_effect_paralyze.cpp
 * @brief Tests for Effect_Paralyze (Thunder Wave, Stun Spore, etc.)
 *
 * These tests verify that status-only paralysis moves work correctly.
 * This is the first non-damage effect we're testing.
 */

#include "framework.hpp"

// Include real implementation headers
#include "../../../source/battle/context.hpp"
#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "../../../source/domain/status.hpp"
// Include common test helpers
#include "../test_helpers.hpp"

// Include real implementation headers
#include "../../../source/battle/effects/basic.hpp"

// ============================================================================

TEST_CASE(Effect_Paralyze_AppliesParalysis) {
    // Setup: Pikachu uses Thunder Wave on Bulbasaur
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Defender should be paralyzed
    TEST_ASSERT_EQ(defender.status1, domain::Status1::PARALYSIS, "Defender should be paralyzed");
}

TEST_CASE(Effect_Paralyze_DoesNotDealDamage) {
    // Setup: Status-only moves deal no damage
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.current_hp = 45;  // Full HP
    auto move = CreateThunderWave();

    uint16_t original_hp = defender.current_hp;
    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: HP unchanged, no damage calculated
    TEST_ASSERT_EQ(defender.current_hp, original_hp, "HP should not change");
    TEST_ASSERT_EQ(ctx.damage_dealt, 0, "No damage should be calculated");
}

TEST_CASE(Effect_Paralyze_DoesNotSetFaintFlag) {
    // Setup: Status moves cannot faint Pokemon
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Pokemon still alive, not fainted
    TEST_ASSERT(!defender.is_fainted, "Pokemon should not be fainted from status move");
    TEST_ASSERT(defender.current_hp > 0, "Pokemon should still have HP");
}

// ============================================================================
// IMMUNITY TESTS
// ============================================================================

TEST_CASE(Effect_Paralyze_AlreadyStatusedFails) {
    // Setup: Pokemon with existing status cannot be paralyzed
    // Run 100 trials to ensure it never succeeds
    for (int i = 0; i < 100; i++) {
        auto test_attacker = CreatePikachu();
        auto test_defender = CreateBulbasaur();
        test_defender.status1 = domain::Status1::BURN;  // Pre-existing burn
        auto test_move = CreateThunderWave();
        auto test_ctx = SetupContext(&test_attacker, &test_defender, &test_move);

        battle::effects::Effect_Paralyze(test_ctx);

        TEST_ASSERT_BATCH(test_defender.status1 == domain::Status1::BURN,
                          "Already-statused Pokemon cannot be paralyzed", 100);
    }
}

TEST_CASE(Effect_Paralyze_AlreadyParalyzedFails) {
    // Setup: Pokemon already paralyzed stays paralyzed
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.status1 = domain::Status1::PARALYSIS;  // Already paralyzed
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Still paralyzed, not double-paralyzed or anything weird
    TEST_ASSERT_EQ(defender.status1, domain::Status1::PARALYSIS,
                   "Already-paralyzed Pokemon stays paralyzed");
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_CASE(Effect_Paralyze_DoesNotModifyAttacker) {
    // Setup: Verify attacker is not affected by using Thunder Wave
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    auto move = CreateThunderWave();

    uint16_t original_hp = attacker.current_hp;
    uint8_t original_status = attacker.status1;

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Attacker unchanged
    TEST_ASSERT_EQ(attacker.current_hp, original_hp, "Attacker HP should not change");
    TEST_ASSERT_EQ(attacker.status1, original_status, "Attacker status should not change");
    TEST_ASSERT(!attacker.is_fainted, "Attacker should not faint");
}

TEST_CASE(Effect_Paralyze_HealthyPokemon) {
    // Setup: Verify paralysis works on full HP Pokemon
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.current_hp = defender.max_hp;  // Full HP
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Paralyzed despite full HP
    TEST_ASSERT_EQ(defender.status1, domain::Status1::PARALYSIS,
                   "Full HP Pokemon can be paralyzed");
    TEST_ASSERT_EQ(defender.current_hp, defender.max_hp, "HP should remain full");
}

TEST_CASE(Effect_Paralyze_LowHpPokemon) {
    // Setup: Verify paralysis works on low HP Pokemon (doesn't KO them)
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.current_hp = 1;  // Very low HP
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Paralyzed and still alive
    TEST_ASSERT_EQ(defender.status1, domain::Status1::PARALYSIS, "Low HP Pokemon can be paralyzed");
    TEST_ASSERT_EQ(defender.current_hp, 1, "HP should remain at 1");
    TEST_ASSERT(!defender.is_fainted, "Should not faint from status move");
}

// ============================================================================
// FUTURE TESTS (commented out until features implemented)
// ============================================================================

/*
TEST_CASE(Effect_Paralyze_GroundTypeImmune) {
    // TODO: Enable when type effectiveness is implemented
    // Ground types are immune to Electric-type Thunder Wave
    auto attacker = CreatePikachu();
    auto defender = CreateGeodude();  // Rock/Ground type
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Ground type should be immune to Thunder Wave");
}

TEST_CASE(Effect_Paralyze_LimberImmune) {
    // TODO: Enable when abilities are implemented
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.ability = Ability::Limber;
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Limber ability prevents paralysis");
}

TEST_CASE(Effect_Paralyze_SafeguardBlocks) {
    // TODO: Enable when field effects are implemented
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    // SetSafeguard(GetSide(defender));
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Safeguard should block paralysis");
}

TEST_CASE(Effect_Paralyze_SubstituteBlocks) {
    // TODO: Enable when Substitute is implemented
    // Unlike burn (Ember), paralysis is blocked by Substitute in Gen III
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.status2 |= STATUS2_SUBSTITUTE;
    defender.substitute_hp = 50;
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Substitute blocks paralysis");
}

TEST_CASE(Effect_Paralyze_MissDoesNotParalyze) {
    // TODO: Enable when accuracy checks are fully implemented
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    SetRNG(99);  // Force miss (Thunder Wave is 100 accuracy, so this is hypothetical)
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT(ctx.move_failed, "Move should have missed");
    TEST_ASSERT_EQ(defender.status1, 0, "Missed move does not paralyze");
}
*/
