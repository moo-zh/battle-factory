/**
 * @file test_effect_hit.cpp
 * @brief Tests for Effect_Hit (basic damage move like Tackle)
 *
 * These tests verify the simplest damage move effect works correctly.
 * This is the foundation that all other damaging effects build upon.
 */

#include "framework.hpp"

// Include real implementation headers
#include "../source/battle/context.hpp"
#include "../source/battle/effects/basic.hpp"
#include "../source/battle/state/pokemon.hpp"
#include "../source/domain/move.hpp"
#include "../source/domain/species.hpp"

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
 * @brief Create Charmander with Gen III base stats
 * Base stats: 39 HP, 52 Atk, 43 Def, 60 SpA, 50 SpD, 65 Spe
 */
battle::state::Pokemon CreateCharmander() {
    return CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire, domain::Type::None,
                             39,   // HP (using base stat as max HP for simplicity)
                             52,   // Attack
                             43,   // Defense
                             60,   // Sp. Attack
                             50,   // Sp. Defense
                             65);  // Speed
}

/**
 * @brief Create Bulbasaur with Gen III base stats
 * Base stats: 45 HP, 49 Atk, 49 Def, 65 SpA, 65 SpD, 45 Spe
 */
battle::state::Pokemon CreateBulbasaur() {
    return CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass, domain::Type::Poison,
                             45,   // HP
                             49,   // Attack
                             49,   // Defense
                             65,   // Sp. Attack
                             65,   // Sp. Defense
                             45);  // Speed
}

/**
 * @brief Create the Tackle move data
 * Gen III: 35 power, 95 accuracy, Normal type
 */
domain::MoveData CreateTackle() {
    domain::MoveData tackle;
    tackle.move = domain::Move::Tackle;
    tackle.type = domain::Type::Normal;
    tackle.power = 35;
    tackle.accuracy = 95;
    tackle.pp = 35;
    tackle.effect_chance = 0;  // No secondary effect
    return tackle;
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
// TESTS
// ============================================================================

TEST_CASE(effect_hit_basic_damage) {
    // Setup: Charmander uses Tackle on Bulbasaur
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTackle();

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Hit(ctx);

    // Assert: Defender should take damage
    TEST_ASSERT(defender.current_hp < defender.max_hp, "Defender HP should decrease");
    TEST_ASSERT(ctx.damage_dealt > 0, "Damage should be calculated");

    // With simplified formula: damage = (22 * 35 * 52 / 49) / 50 + 2
    // = (40040 / 49) / 50 + 2 = 817 / 50 + 2 = 16 + 2 = 18 damage
    // This is approximate - we're testing it works, not exact values yet
    TEST_ASSERT(ctx.damage_dealt > 10, "Damage should be reasonable (> 10)");
    TEST_ASSERT(ctx.damage_dealt < 30, "Damage should be reasonable (< 30)");
}

TEST_CASE(effect_hit_damage_scales_with_attack) {
    // Setup: Two attackers with different Attack stats
    auto weak_attacker = CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire,
                                           domain::Type::None, 100, 30, 40, 50, 50, 50);

    auto strong_attacker = CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire,
                                             domain::Type::None, 100, 90, 40, 50, 50, 50);

    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto move = CreateTackle();

    // Test weak attacker
    auto ctx1 = SetupContext(&weak_attacker, &defender1, &move);
    battle::effects::Effect_Hit(ctx1);

    // Test strong attacker
    auto ctx2 = SetupContext(&strong_attacker, &defender2, &move);
    battle::effects::Effect_Hit(ctx2);

    // Assert: Higher attack = more damage
    TEST_ASSERT(ctx2.damage_dealt > ctx1.damage_dealt, "Higher Attack should deal more damage");
}

TEST_CASE(effect_hit_damage_scales_with_defense) {
    // Setup: Two defenders with different Defense stats
    auto attacker1 = CreateCharmander();
    auto attacker2 = CreateCharmander();

    auto weak_defender = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                           domain::Type::Poison, 100, 50, 20, 50, 50, 50);

    auto strong_defender = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                             domain::Type::Poison, 100, 50, 80, 50, 50, 50);

    auto move = CreateTackle();

    // Test weak defender
    auto ctx1 = SetupContext(&attacker1, &weak_defender, &move);
    battle::effects::Effect_Hit(ctx1);

    // Test strong defender
    auto ctx2 = SetupContext(&attacker2, &strong_defender, &move);
    battle::effects::Effect_Hit(ctx2);

    // Assert: Higher defense = less damage
    TEST_ASSERT(ctx1.damage_dealt > ctx2.damage_dealt, "Higher Defense should reduce damage");
}

TEST_CASE(effect_hit_can_ko) {
    // Setup: Weak defender with 1 HP
    auto attacker = CreateCharmander();
    auto weak_defender = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                           domain::Type::Poison, 100, 50, 50, 50, 50, 50);

    weak_defender.current_hp = 1;  // Set to 1 HP
    weak_defender.is_fainted = false;

    auto move = CreateTackle();
    auto ctx = SetupContext(&attacker, &weak_defender, &move);

    // Execute
    battle::effects::Effect_Hit(ctx);

    // Assert: Defender should faint
    TEST_ASSERT_EQ(weak_defender.current_hp, 0, "HP should be 0 after KO");
    TEST_ASSERT(weak_defender.is_fainted, "Faint flag should be set");
}

TEST_CASE(effect_hit_minimum_damage) {
    // Setup: Extremely weak attacker vs. extremely strong defender
    auto weak_attacker = CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire,
                                           domain::Type::None, 100, 5, 50, 50, 50, 50);

    auto tank_defender = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                           domain::Type::Poison, 100, 50, 200, 50, 50, 50);

    auto move = CreateTackle();
    auto ctx = SetupContext(&weak_attacker, &tank_defender, &move);

    // Execute
    battle::effects::Effect_Hit(ctx);

    // Assert: Should still deal at least 1 damage (Gen III minimum)
    TEST_ASSERT(ctx.damage_dealt >= 1, "Minimum damage should be 1");
}

TEST_CASE(effect_hit_hp_clamped_at_zero) {
    // Setup: Overkill damage (more than current HP)
    auto attacker = CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire,
                                      domain::Type::None, 100, 200, 50, 50, 50, 50);

    auto defender = CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass,
                                      domain::Type::Poison, 10, 50, 50, 50, 50, 50);

    auto move = CreateTackle();
    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Hit(ctx);

    // Assert: HP should not go negative
    TEST_ASSERT_EQ(defender.current_hp, 0, "HP should be clamped at 0");
    TEST_ASSERT(defender.is_fainted, "Pokemon should be fainted");
}

TEST_CASE(effect_hit_does_not_modify_attacker) {
    // Setup
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateTackle();

    uint16_t original_hp = attacker.current_hp;
    bool original_fainted = attacker.is_fainted;

    auto ctx = SetupContext(&attacker, &defender, &move);

    // Execute
    battle::effects::Effect_Hit(ctx);

    // Assert: Attacker should not take damage (this is not recoil)
    TEST_ASSERT_EQ(attacker.current_hp, original_hp, "Attacker HP should not change");
    TEST_ASSERT_EQ(attacker.is_fainted, original_fainted, "Attacker faint state should not change");
}

// ============================================================================
// TODO: Tests for future features (commented out until implemented)
// ============================================================================

// These tests are documented but not run yet - we'll enable them as we
// add features in subsequent moves

/*
TEST_CASE(effect_hit_can_miss) {
    // TODO: Enable when accuracy formula is implemented
    // Test that moves with < 100 accuracy can miss
    // Will need to mock RNG or run many iterations
}

TEST_CASE(effect_hit_miss_deals_no_damage) {
    // TODO: Enable when accuracy formula is implemented
    // Verify that on miss, no damage is dealt and move_failed is set
}

TEST_CASE(effect_hit_type_immunity) {
    // TODO: Enable when type effectiveness is implemented
    // Normal vs. Ghost should deal 0 damage
}

TEST_CASE(effect_hit_super_effective) {
    // TODO: Enable when type effectiveness is implemented
    // Fire vs. Grass should deal 2x damage
}

TEST_CASE(effect_hit_not_very_effective) {
    // TODO: Enable when type effectiveness is implemented
    // Water vs. Grass should deal 0.5x damage
}

TEST_CASE(effect_hit_critical_hit) {
    // TODO: Enable when critical hits are implemented
    // Verify crits deal 2x damage
    // Verify ctx.critical_hit flag is set
}

TEST_CASE(effect_hit_stab) {
    // TODO: Enable when STAB is implemented
    // Fire-type using Fire move should get 1.5x boost
}

TEST_CASE(effect_hit_random_variance) {
    // TODO: Enable when random variance is added
    // Same attack should produce damage in 85-100% range
}
*/
