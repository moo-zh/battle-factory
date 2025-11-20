/**
 * @file test_effect_paralyze.cpp
 * @brief Tests for Effect_Paralyze (Thunder Wave, Stun Spore, etc.)
 *
 * These tests verify that status-only paralysis moves work correctly.
 * This is the first non-damage effect we're testing.
 */

#include "framework.hpp"

// Include real implementation headers
#include "../source/battle/context.hpp"
#include "../source/battle/effects/basic.hpp"
#include "../source/battle/state/pokemon.hpp"
#include "../source/domain/move.hpp"
#include "../source/domain/species.hpp"
#include "../source/domain/status.hpp"

// ============================================================================
// TEST HELPERS
// ============================================================================

/**
 * @brief Create a Pokemon for testing with specified stats
 */
battle::state::Pokemon CreateTestPokemon(domain::Species species, domain::Type type1,
                                         domain::Type type2, uint16_t hp, uint8_t atk, uint8_t def,
                                         uint8_t spa, uint8_t spd, uint8_t spe) {
    battle::state::Pokemon p;
    p.species = species;
    p.type1 = type1;
    p.type2 = type2;
    p.attack = atk;
    p.defense = def;
    p.sp_attack = spa;
    p.sp_defense = spd;
    p.speed = spe;
    p.max_hp = hp;
    p.current_hp = hp;
    p.is_fainted = false;
    p.status1 = 0;  // No status
    return p;
}

/**
 * @brief Create Pikachu with Gen III base stats
 * Base stats: 35 HP, 55 Atk, 40 Def, 50 SpA, 50 SpD, 90 Spe
 */
battle::state::Pokemon CreatePikachu() {
    return CreateTestPokemon(domain::Species::Pikachu, domain::Type::Electric, domain::Type::None,
                             35, 55, 40, 50, 50, 90);
}

/**
 * @brief Create Bulbasaur with Gen III base stats
 * Base stats: 45 HP, 49 Atk, 49 Def, 65 SpA, 65 SpD, 45 Spe
 */
battle::state::Pokemon CreateBulbasaur() {
    return CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass, domain::Type::Poison,
                             45, 49, 49, 65, 65, 45);
}

/**
 * @brief Create Geodude with Gen III base stats
 * Base stats: 40 HP, 80 Atk, 100 Def, 30 SpA, 30 SpD, 20 Spe
 */
battle::state::Pokemon CreateGeodude() {
    return CreateTestPokemon(domain::Species::Geodude, domain::Type::Rock, domain::Type::Ground,
                             40, 80, 100, 30, 30, 20);
}

/**
 * @brief Create the Thunder Wave move data
 * Gen III: 0 power, 100 accuracy, Electric type
 */
domain::MoveData CreateThunderWave() {
    domain::MoveData twave;
    twave.move = domain::Move::ThunderWave;
    twave.type = domain::Type::Electric;
    twave.power = 0;
    twave.accuracy = 100;
    twave.pp = 20;
    twave.effect_chance = 0;  // Not used for 100% effect moves
    return twave;
}

/**
 * @brief Setup a battle context for testing
 */
battle::BattleContext SetupContext(battle::state::Pokemon* attacker,
                                   battle::state::Pokemon* defender, const domain::MoveData* move) {
    battle::BattleContext ctx;
    ctx.attacker = attacker;
    ctx.defender = defender;
    ctx.move = move;
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // Default to 1x (neutral)
    ctx.override_power = 0;
    ctx.override_type = 0;
    return ctx;
}

// ============================================================================
// BASIC FUNCTIONALITY TESTS
// ============================================================================

TEST_CASE(effect_paralyze_applies_paralysis) {
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

TEST_CASE(effect_paralyze_does_not_deal_damage) {
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

TEST_CASE(effect_paralyze_does_not_set_faint_flag) {
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

TEST_CASE(effect_paralyze_already_statused_fails) {
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

TEST_CASE(effect_paralyze_already_paralyzed_fails) {
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

TEST_CASE(effect_paralyze_does_not_modify_attacker) {
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

TEST_CASE(effect_paralyze_healthy_pokemon) {
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

TEST_CASE(effect_paralyze_low_hp_pokemon) {
    // Setup: Verify paralysis works on low HP Pokemon (doesn't KO them)
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.current_hp = 1;  // Very low HP
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Paralyze(ctx);

    // Assert: Paralyzed and still alive
    TEST_ASSERT_EQ(defender.status1, domain::Status1::PARALYSIS,
                   "Low HP Pokemon can be paralyzed");
    TEST_ASSERT_EQ(defender.current_hp, 1, "HP should remain at 1");
    TEST_ASSERT(!defender.is_fainted, "Should not faint from status move");
}

// ============================================================================
// FUTURE TESTS (commented out until features implemented)
// ============================================================================

/*
TEST_CASE(effect_paralyze_ground_type_immune) {
    // TODO: Enable when type effectiveness is implemented
    // Ground types are immune to Electric-type Thunder Wave
    auto attacker = CreatePikachu();
    auto defender = CreateGeodude();  // Rock/Ground type
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Ground type should be immune to Thunder Wave");
}

TEST_CASE(effect_paralyze_limber_immune) {
    // TODO: Enable when abilities are implemented
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    defender.ability = Ability::Limber;
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Limber ability prevents paralysis");
}

TEST_CASE(effect_paralyze_safeguard_blocks) {
    // TODO: Enable when field effects are implemented
    auto attacker = CreatePikachu();
    auto defender = CreateBulbasaur();
    // SetSafeguard(GetSide(defender));
    auto move = CreateThunderWave();

    auto ctx = SetupContext(&attacker, &defender, &move);
    battle::effects::Effect_Paralyze(ctx);

    TEST_ASSERT_EQ(defender.status1, 0, "Safeguard should block paralysis");
}

TEST_CASE(effect_paralyze_substitute_blocks) {
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

TEST_CASE(effect_paralyze_miss_does_not_paralyze) {
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
