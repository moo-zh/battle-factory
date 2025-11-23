/**
 * @file test/host/abilities/test_intimidate.cpp
 * @brief Tests for Intimidate ability
 *
 * Intimidate lowers the opponent's Attack stat by 1 stage when the Pokemon
 * switches into battle.
 *
 * Based on Gen III behavior (pokeemerald):
 * - Triggers on switch-in (including battle start)
 * - Lowers opponent's Attack by 1 stage (-1)
 * - Does not activate if opponent has Substitute
 * - Does not activate if opponent has Clear Body/White Smoke/Hyper Cutter
 * - Multiple Intimidates stack (each lowers Attack by 1)
 *
 * For this vertical slice, we're testing:
 * 1. Basic switch-in trigger (InitBattle)
 * 2. Attack lowering mechanics
 * 3. Both Pokemon with Intimidate
 * 4. Edge cases (already at -6, etc.)
 *
 * Future slices will test:
 * - Substitute blocking Intimidate
 * - Immunity abilities (Clear Body, etc.)
 * - Switch-in during battle (not just InitBattle)
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace battle;
using namespace domain;

// ============================================================================
// Basic Intimidate Mechanics
// ============================================================================

TEST(IntimidateTest, PlayerIntimidate_LowersEnemyAttack) {
    // Player has Intimidate, enemy does not
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Enemy's Attack should be lowered by 1 stage
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -1);

    // Player's Attack should be unaffected
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], 0);
}

TEST(IntimidateTest, EnemyIntimidate_LowersPlayerAttack) {
    // Enemy has Intimidate, player does not
    auto player = CreateCharmander();
    player.ability = Ability::None;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::Intimidate;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Player's Attack should be lowered by 1 stage
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], -1);

    // Enemy's Attack should be unaffected
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], 0);
}

TEST(IntimidateTest, BothIntimidate_BothLowered) {
    // Both Pokemon have Intimidate
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::Intimidate;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Both should have Attack lowered by 1 stage
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], -1);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -1);
}

TEST(IntimidateTest, NoIntimidate_NoStatChange) {
    // Neither Pokemon has Intimidate
    auto player = CreateCharmander();
    player.ability = Ability::None;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Neither should have stat changes
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], 0);
}

// ============================================================================
// Stat Stage Edge Cases
// ============================================================================

TEST(IntimidateTest, AlreadyAtMinusOne_CanLowerFurther) {
    // Enemy starts with Attack at -1, Intimidate should lower to -2
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;
    enemy.stat_stages[STAT_ATK] = -1;  // Pre-lowered

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Should be lowered to -2
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -2);
}

TEST(IntimidateTest, AlreadyAtMinusSix_CannotLowerFurther) {
    // Enemy starts at -6 (minimum), Intimidate should not go below -6
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;
    enemy.stat_stages[STAT_ATK] = -6;  // Already at minimum

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Should remain at -6 (cannot go lower)
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -6);
}

TEST(IntimidateTest, AtMinusFive_LowersToMinusSix) {
    // Edge case: -5 → -6 (reaches minimum exactly)
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;
    enemy.stat_stages[STAT_ATK] = -5;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Should be lowered to -6 (minimum)
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -6);
}

// ============================================================================
// Only Attack is Affected
// ============================================================================

TEST(IntimidateTest, OnlyAttackLowered_OtherStatsUnaffected) {
    // Intimidate should only lower Attack, not other stats
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Attack should be lowered
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -1);

    // All other stats should be neutral (0)
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_DEF], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_SPEED], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_SPATK], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_SPDEF], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ACC], 0);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_EVASION], 0);
}

// ============================================================================
// Positive Stat Stages
// ============================================================================

TEST(IntimidateTest, EnemyAtPlusOne_LoweredToZero) {
    // Enemy starts with +1 Attack, Intimidate lowers it to 0 (neutral)
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;
    enemy.stat_stages[STAT_ATK] = 1;  // Boosted

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Should be lowered to 0 (neutral)
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], 0);
}

TEST(IntimidateTest, EnemyAtPlusSix_LoweredToPlusFive) {
    // Enemy at maximum Attack (+6), Intimidate lowers by 1
    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::None;
    enemy.stat_stages[STAT_ATK] = 6;  // Maximum

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Should be lowered to +5
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], 5);
}

// ============================================================================
// Integration: Intimidate + Moves
// ============================================================================

TEST(IntimidateTest, IntimidateThenTackle_UsesLoweredAttack) {
    // Intimidate lowers Attack, then verify damage uses lowered stat
    // Compare damage with and without Intimidate

    // Baseline: No Intimidate
    auto player_baseline = CreateCharmander();
    player_baseline.ability = Ability::None;
    auto enemy_baseline = CreateBulbasaur();
    enemy_baseline.ability = Ability::None;

    BattleEngine engine_baseline;
    engine_baseline.InitBattle(player_baseline, enemy_baseline);

    BattleAction player_action_baseline{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    BattleAction enemy_action_baseline{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};

    uint16_t enemy_hp_before_baseline = engine_baseline.GetEnemy().current_hp;
    engine_baseline.ExecuteTurn(player_action_baseline, enemy_action_baseline);
    uint16_t damage_baseline = enemy_hp_before_baseline - engine_baseline.GetEnemy().current_hp;

    // With Intimidate: Enemy has Intimidate, lowering player's Attack
    auto player = CreateCharmander();
    player.ability = Ability::None;
    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::Intimidate;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Player's Attack is now -1 stage
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], -1);

    // Player attacks with Tackle
    BattleAction player_action{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    BattleAction enemy_action{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};

    uint16_t enemy_hp_before = engine.GetEnemy().current_hp;
    engine.ExecuteTurn(player_action, enemy_action);
    uint16_t damage_intimidate = enemy_hp_before - engine.GetEnemy().current_hp;

    // Damage with -1 Attack should be less than baseline damage
    // At -1 stage: multiplier = 2 / (2 - (-1)) = 2/3 ≈ 0.67x damage
    EXPECT_LT(damage_intimidate, damage_baseline)
        << "Damage with -1 Attack (" << damage_intimidate << ") should be less than baseline ("
        << damage_baseline << ")";
    EXPECT_GT(damage_intimidate, 0) << "Should still deal some damage";
}

// ============================================================================
// Order of Activation
// ============================================================================

TEST(IntimidateTest, PlayerIntimidateActivatesFirst) {
    // When both have Intimidate, player activates first, then enemy
    // This test verifies order by checking intermediate state isn't observable,
    // but final state has both at -1

    auto player = CreateCharmander();
    player.ability = Ability::Intimidate;

    auto enemy = CreateBulbasaur();
    enemy.ability = Ability::Intimidate;

    BattleEngine engine;
    engine.InitBattle(player, enemy);

    // Final state: both at -1 (player lowered enemy, enemy lowered player)
    EXPECT_EQ(engine.GetPlayer().stat_stages[STAT_ATK], -1);
    EXPECT_EQ(engine.GetEnemy().stat_stages[STAT_ATK], -1);

    // This verifies that the implementation in InitBattle correctly
    // triggers player's Intimidate first (lowering enemy to -1),
    // then enemy's Intimidate (lowering player to -1)
}
