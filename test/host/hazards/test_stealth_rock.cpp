/**
 * @file test/host/hazards/test_stealth_rock.cpp
 * @brief Tests for Stealth Rock entry hazard
 *
 * This file tests Stealth Rock mechanics:
 * - Setting hazard on opponent's side
 * - Type effectiveness scaling (4x, 2x, 1x, 0.5x, 0.25x)
 * - Switch-in damage application
 * - Hazard persistence
 * - Preventing re-application
 *
 * Part of Stealth Rock vertical slice implementation.
 */

#include <gtest/gtest.h>

#include "battle/commands/hazards.hpp"
#include "battle/engine.hpp"
#include "test_common.hpp"

/**
 * @brief Test fixture for Stealth Rock tests
 */
class StealthRockTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
    battle::state::Side side;
};

// ============================================================================
// Hazard Application Tests
// ============================================================================

TEST_F(StealthRockTest, SetHazard_SetsStealthRockFlag) {
    // Initially no hazard
    side.stealth_rock = false;

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    // Use Stealth Rock
    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::StealthRock};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Tackle};

    engine.ExecuteTurn(player_action, enemy_action);

    // Check that enemy side has stealth rock (player used it, so it's on enemy side)
    // Note: We can't access engine's internal side state directly from tests yet
    // So we'll test via the effect function directly

    domain::MoveData sr = {domain::Move::StealthRock, domain::Type::Rock, 0, 0, 20, 0, 0};
    battle::BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.attacker_side = &side;  // Dummy
    ctx.defender_side = &side;  // Target side
    ctx.move = &sr;
    ctx.move_failed = false;

    battle::effects::Effect_StealthRock(ctx);

    EXPECT_TRUE(side.stealth_rock) << "Stealth Rock should set hazard flag";
    EXPECT_FALSE(ctx.move_failed) << "Stealth Rock should succeed";
}

TEST_F(StealthRockTest, SetHazard_FailsIfAlreadySet) {
    // Hazard already set
    side.stealth_rock = true;

    domain::MoveData sr = {domain::Move::StealthRock, domain::Type::Rock, 0, 0, 20, 0, 0};
    battle::BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.attacker_side = &side;
    ctx.defender_side = &side;
    ctx.move = &sr;
    ctx.move_failed = false;

    battle::effects::Effect_StealthRock(ctx);

    EXPECT_TRUE(ctx.move_failed) << "Stealth Rock should fail if already set";
    EXPECT_TRUE(side.stealth_rock) << "Stealth Rock flag should remain set";
}

// ============================================================================
// Switch-In Damage Tests (Type Effectiveness)
// ============================================================================

TEST_F(StealthRockTest, SwitchIn_NeutralDamage_1_8thMaxHP) {
    // Bulbasaur (Grass/Poison) vs Rock = 1x (neutral)
    side.stealth_rock = true;
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::commands::ApplyStealthRockDamage(defender, side);

    // Based on pokeemerald: (max_hp / 8) * type_effectiveness
    // Neutral (1x): (100 * 4) / 32 = 12 HP damage
    EXPECT_EQ(defender.current_hp, 88) << "Neutral effectiveness should deal 1/8 max HP (12 HP)";
    EXPECT_FALSE(defender.is_fainted) << "Should not faint from switch-in";
}

TEST_F(StealthRockTest, SwitchIn_SuperEffective_2x_QuarterMaxHP) {
    // Charizard (Fire/Flying) vs Rock = 4x
    battle::state::Pokemon charizard = CreateCharizard();
    charizard.max_hp = 100;
    charizard.current_hp = 100;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(charizard, side);

    // Based on pokeemerald: (max_hp / 8) * type_effectiveness
    // 4x weakness: (100 * 16) / 32 = 50 HP damage (50% max HP)
    EXPECT_EQ(charizard.current_hp, 50) << "4x super effective should deal 50% max HP";
}

TEST_F(StealthRockTest, SwitchIn_DoubleResist_OneThirtySecondMaxHP) {
    // Create Fighting/Steel type (double resists Rock: 0.25x)
    battle::state::Pokemon fighter = CreatePikachu();  // Reuse, change types
    fighter.type1 = domain::Type::Fighting;
    fighter.type2 = domain::Type::Steel;
    fighter.max_hp = 128;
    fighter.current_hp = 128;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(fighter, side);

    // Expected: (128 / 8) * 0.25 = 16 * 0.25 = 4 HP damage
    // In fixed-point: (16 * 1) / 4 = 4 HP
    EXPECT_EQ(fighter.current_hp, 124) << "0.25x resist should deal 1/32 max HP (4 HP)";
}

TEST_F(StealthRockTest, SwitchIn_Resist_OneSixteenthMaxHP) {
    // Fighting type vs Rock = 0.5x
    battle::state::Pokemon fighter = CreatePikachu();
    fighter.type1 = domain::Type::Fighting;
    fighter.type2 = domain::Type::None;  // Monotype
    fighter.max_hp = 96;
    fighter.current_hp = 96;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(fighter, side);

    // Expected: (96 * 2) / 32 = 192 / 32 = 6 HP damage
    EXPECT_EQ(fighter.current_hp, 90) << "0.5x resist should deal 1/16 max HP (6 HP)";
}

TEST_F(StealthRockTest, SwitchIn_Fire_TypeSuperEffective) {
    // Charmander (Fire) vs Rock = 2x
    battle::state::Pokemon charmander = CreateCharmander();
    charmander.max_hp = 80;
    charmander.current_hp = 80;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(charmander, side);

    // Expected: (80 / 8) * 2 = 10 * 2 = 20 HP damage
    // In fixed-point: (10 * 8) / 4 = 20 HP
    EXPECT_EQ(charmander.current_hp, 60) << "2x effectiveness should deal 1/4 max HP (20 HP)";
}

TEST_F(StealthRockTest, SwitchIn_Flying_TypeSuperEffective) {
    // Create pure Flying type
    battle::state::Pokemon bird = CreatePikachu();
    bird.type1 = domain::Type::Flying;
    bird.type2 = domain::Type::None;  // Monotype
    bird.max_hp = 80;
    bird.current_hp = 80;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(bird, side);

    // Expected: (80 * 8) / 32 = 640 / 32 = 20 HP damage
    EXPECT_EQ(bird.current_hp, 60) << "Flying type should take 2x damage";
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(StealthRockTest, SwitchIn_NoHazard_NoDamage) {
    // No stealth rock set
    side.stealth_rock = false;
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 100) << "No damage if stealth rock not set";
}

TEST_F(StealthRockTest, SwitchIn_AlreadyFainted_NoDamage) {
    // Pokemon already fainted
    side.stealth_rock = true;
    defender.current_hp = 0;
    defender.is_fainted = true;
    defender.max_hp = 100;

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 0) << "Already fainted Pokemon should not take damage";
}

TEST_F(StealthRockTest, SwitchIn_LowHP_Faints) {
    // Pokemon with low HP faints from stealth rock
    side.stealth_rock = true;
    defender.max_hp = 100;
    defender.current_hp = 10;  // Less than 12 HP damage

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 0) << "Should faint if damage >= current HP";
    EXPECT_TRUE(defender.is_fainted) << "Should be marked as fainted";
}

TEST_F(StealthRockTest, SwitchIn_ExactLethalDamage) {
    // Damage exactly equals current HP
    side.stealth_rock = true;
    defender.max_hp = 96;  // 96/8 = 12 HP damage (neutral)
    defender.current_hp = 12;

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 0) << "Exact lethal damage should faint";
    EXPECT_TRUE(defender.is_fainted) << "Should be marked as fainted";
}

TEST_F(StealthRockTest, SwitchIn_RoundingDown_SmallHP) {
    // Max HP < 8 results in 0 damage (integer division)
    side.stealth_rock = true;
    defender.max_hp = 7;
    defender.current_hp = 7;

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 7) << "Max HP < 8 should deal 0 damage (rounds down)";
}

TEST_F(StealthRockTest, SwitchIn_MinimumDamage_EdgeCase) {
    // Very small max HP with super effective
    side.stealth_rock = true;
    battle::state::Pokemon weak = CreateCharmander();
    weak.type1 = domain::Type::Fire;
    weak.type2 = domain::Type::Flying;
    weak.max_hp = 16;  // 16/8 = 2, 2*4 = 8 damage
    weak.current_hp = 16;

    battle::commands::ApplyStealthRockDamage(weak, side);

    // (16 / 8) * 4 = 2 * 4 = 8 HP damage
    // Fixed-point: (2 * 16) / 4 = 8
    EXPECT_EQ(weak.current_hp, 8) << "Small HP with 4x should still deal correct damage";
}

// ============================================================================
// Type Effectiveness Validation Tests
// ============================================================================

TEST_F(StealthRockTest, TypeChart_RockVsGround_NotVeryEffective) {
    // Ground resists Rock (0.5x)
    battle::state::Pokemon ground = CreatePikachu();
    ground.type1 = domain::Type::Ground;
    ground.type2 = domain::Type::None;  // Monotype
    ground.max_hp = 96;
    ground.current_hp = 96;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(ground, side);

    // (96 * 2) / 32 = 192 / 32 = 6 HP
    EXPECT_EQ(ground.current_hp, 90) << "Ground should resist Rock (0.5x)";
}

TEST_F(StealthRockTest, TypeChart_RockVsSteel_NotVeryEffective) {
    // Steel resists Rock (0.5x)
    battle::state::Pokemon steel = CreatePikachu();
    steel.type1 = domain::Type::Steel;
    steel.type2 = domain::Type::None;  // Monotype
    steel.max_hp = 96;
    steel.current_hp = 96;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(steel, side);

    // (96 * 2) / 32 = 192 / 32 = 6 HP
    EXPECT_EQ(steel.current_hp, 90) << "Steel should resist Rock (0.5x)";
}

TEST_F(StealthRockTest, TypeChart_RockVsIce_SuperEffective) {
    // Ice weak to Rock (2x)
    battle::state::Pokemon ice = CreatePikachu();
    ice.type1 = domain::Type::Ice;
    ice.type2 = domain::Type::None;  // Monotype
    ice.max_hp = 80;
    ice.current_hp = 80;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(ice, side);

    // (80 * 8) / 32 = 640 / 32 = 20 HP
    EXPECT_EQ(ice.current_hp, 60) << "Ice should be weak to Rock (2x)";
}

TEST_F(StealthRockTest, TypeChart_RockVsBug_SuperEffective) {
    // Bug weak to Rock (2x)
    battle::state::Pokemon bug = CreatePikachu();
    bug.type1 = domain::Type::Bug;
    bug.type2 = domain::Type::None;  // Monotype
    bug.max_hp = 80;
    bug.current_hp = 80;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(bug, side);

    // (80 * 8) / 32 = 640 / 32 = 20 HP
    EXPECT_EQ(bug.current_hp, 60) << "Bug should be weak to Rock (2x)";
}

// ============================================================================
// Relationship Tests (Behavioral Contracts)
// ============================================================================

TEST_F(StealthRockTest, Relationship_SuperEffectiveGreaterThanNeutral) {
    // 2x effectiveness should deal more damage than 1x
    battle::state::Pokemon weak_mon = CreateCharmander();    // Fire: 2x weak to Rock
    battle::state::Pokemon neutral_mon = CreateBulbasaur();  // Grass/Poison: 1x to Rock
    weak_mon.max_hp = 100;
    weak_mon.current_hp = 100;
    neutral_mon.max_hp = 100;
    neutral_mon.current_hp = 100;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(weak_mon, side);
    battle::commands::ApplyStealthRockDamage(neutral_mon, side);

    uint16_t weak_damage = 100 - weak_mon.current_hp;
    uint16_t neutral_damage = 100 - neutral_mon.current_hp;

    EXPECT_GT(weak_damage, neutral_damage) << "Super effective should deal more than neutral";
}

TEST_F(StealthRockTest, Relationship_NeutralGreaterThanResist) {
    // 1x effectiveness should deal more damage than 0.5x
    battle::state::Pokemon neutral_mon = CreateBulbasaur();  // Neutral to Rock
    battle::state::Pokemon resist_mon = CreatePikachu();
    resist_mon.type1 = domain::Type::Fighting;  // Resists Rock (0.5x)
    resist_mon.type2 = domain::Type::None;
    neutral_mon.max_hp = 100;
    neutral_mon.current_hp = 100;
    resist_mon.max_hp = 100;
    resist_mon.current_hp = 100;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(neutral_mon, side);
    battle::commands::ApplyStealthRockDamage(resist_mon, side);

    uint16_t neutral_damage = 100 - neutral_mon.current_hp;
    uint16_t resist_damage = 100 - resist_mon.current_hp;

    EXPECT_GT(neutral_damage, resist_damage) << "Neutral should deal more than resist";
}

TEST_F(StealthRockTest, Relationship_DoubleWeaknessGreaterThanSingleWeakness) {
    // 4x effectiveness should deal more damage than 2x
    battle::state::Pokemon double_weak = CreateCharizard();   // Fire/Flying: 4x weak
    battle::state::Pokemon single_weak = CreateCharmander();  // Fire: 2x weak
    double_weak.max_hp = 100;
    double_weak.current_hp = 100;
    single_weak.max_hp = 100;
    single_weak.current_hp = 100;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(double_weak, side);
    battle::commands::ApplyStealthRockDamage(single_weak, side);

    uint16_t double_damage = 100 - double_weak.current_hp;
    uint16_t single_damage = 100 - single_weak.current_hp;

    EXPECT_GT(double_damage, single_damage) << "4x weak should deal more than 2x weak";
}

TEST_F(StealthRockTest, Relationship_ResistGreaterThanDoubleResist) {
    // 0.5x effectiveness should deal more damage than 0.25x
    battle::state::Pokemon single_resist = CreatePikachu();
    single_resist.type1 = domain::Type::Fighting;  // 0.5x to Rock
    single_resist.type2 = domain::Type::None;
    battle::state::Pokemon double_resist = CreatePikachu();
    double_resist.type1 = domain::Type::Fighting;  // 0.25x to Rock (Fighting/Steel)
    double_resist.type2 = domain::Type::Steel;
    single_resist.max_hp = 128;
    single_resist.current_hp = 128;
    double_resist.max_hp = 128;
    double_resist.current_hp = 128;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(single_resist, side);
    battle::commands::ApplyStealthRockDamage(double_resist, side);

    uint16_t single_damage = 128 - single_resist.current_hp;
    uint16_t double_damage = 128 - double_resist.current_hp;

    EXPECT_GT(single_damage, double_damage) << "0.5x resist should deal more than 0.25x resist";
}

TEST_F(StealthRockTest, Relationship_DamageScalesWithMaxHP) {
    // Same type effectiveness, higher max HP = higher absolute damage
    battle::state::Pokemon low_hp = CreateBulbasaur();
    battle::state::Pokemon high_hp = CreateBulbasaur();
    low_hp.max_hp = 50;
    low_hp.current_hp = 50;
    high_hp.max_hp = 200;
    high_hp.current_hp = 200;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(low_hp, side);
    battle::commands::ApplyStealthRockDamage(high_hp, side);

    uint16_t low_damage = 50 - low_hp.current_hp;
    uint16_t high_damage = 200 - high_hp.current_hp;

    EXPECT_GT(high_damage, low_damage) << "Higher max HP should take more absolute damage";
}

TEST_F(StealthRockTest, Relationship_AnyDamageGreaterThanImmune) {
    // Any non-immune Pokemon takes damage, immune takes 0
    // Note: Rock has no immunities in Gen III, so this is a structural test
    battle::state::Pokemon normal_mon = CreateBulbasaur();
    normal_mon.max_hp = 100;
    normal_mon.current_hp = 100;
    side.stealth_rock = true;

    battle::commands::ApplyStealthRockDamage(normal_mon, side);

    uint16_t damage = 100 - normal_mon.current_hp;

    EXPECT_GT(damage, 0) << "Non-immune Pokemon should take damage";
}

// ============================================================================
// Integration Test
// ============================================================================

TEST_F(StealthRockTest, Integration_SetAndApply) {
    // Set hazard, then apply damage
    side.stealth_rock = false;

    // Step 1: Set Stealth Rock
    domain::MoveData sr = {domain::Move::StealthRock, domain::Type::Rock, 0, 0, 20, 0, 0};
    battle::BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.attacker_side = &side;
    ctx.defender_side = &side;
    ctx.move = &sr;
    ctx.move_failed = false;

    battle::effects::Effect_StealthRock(ctx);

    EXPECT_TRUE(side.stealth_rock) << "Hazard should be set";

    // Step 2: Apply switch-in damage
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::commands::ApplyStealthRockDamage(defender, side);

    EXPECT_EQ(defender.current_hp, 88) << "Should take 12 HP damage on switch-in";
}
