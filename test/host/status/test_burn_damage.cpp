/**
 * @file test/host/status/test_burn_damage.cpp
 * @brief Tests for burn status damage-over-time and attack reduction
 *
 * This file tests burn's persistent effects:
 * - End-of-turn damage (1/8 max HP per turn)
 * - Attack reduction (50% of attack stat)
 * - Integration with battle flow
 *
 * Part of burn vertical slice implementation.
 */

#include <gtest/gtest.h>

#include "battle/engine.hpp"
#include "test_common.hpp"

/**
 * @brief Test fixture for burn damage tests
 */
class BurnDamageTest : public ::testing::Test {
   protected:
    void SetUp() override {
        battle::random::Initialize(42);
        attacker = CreateCharmander();
        defender = CreateBulbasaur();
    }

    battle::state::Pokemon attacker;
    battle::state::Pokemon defender;
};

// ============================================================================
// End-of-Turn Damage Tests
// ============================================================================

TEST_F(BurnDamageTest, EndOfTurn_DealsDamageEachTurn) {
    // Setup: Burned Pokemon with known max HP
    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 100;
    defender.current_hp = 100;

    // Create engine and execute a turn
    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    // Execute a turn where both Pokemon do nothing (to trigger end-of-turn)
    // Use Growl (non-damaging) to isolate burn damage
    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    engine.ExecuteTurn(player_action, enemy_action);

    // Verify: Enemy should have taken burn damage (1/8 of 100 = 12 HP)
    const battle::state::Pokemon& enemy = engine.GetEnemy();
    EXPECT_EQ(enemy.current_hp, 100 - 12) << "Should lose 1/8 max HP (12 HP) to burn";
}

TEST_F(BurnDamageTest, EndOfTurn_Damage1_8thMaxHP) {
    // Test with different max HP values
    struct TestCase {
        uint16_t max_hp;
        uint16_t expected_damage;
    };

    TestCase test_cases[] = {
        {8, 1},     // 8/8 = 1
        {16, 2},    // 16/8 = 2
        {24, 3},    // 24/8 = 3
        {100, 12},  // 100/8 = 12
        {200, 25},  // 200/8 = 25
        {7, 0},     // 7/8 = 0 (rounds down)
        {1, 0},     // 1/8 = 0
    };

    for (const auto& tc : test_cases) {
        battle::state::Pokemon test_attacker = CreateCharmander();
        battle::state::Pokemon test_defender = CreateBulbasaur();
        test_defender.status1 = domain::Status1::BURN;
        test_defender.max_hp = tc.max_hp;
        test_defender.current_hp = tc.max_hp;

        battle::BattleEngine engine;
        engine.InitBattle(test_attacker, test_defender);

        battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                           domain::Move::Growl};
        battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                          domain::Move::Growl};

        engine.ExecuteTurn(player_action, enemy_action);

        const battle::state::Pokemon& enemy = engine.GetEnemy();
        uint16_t expected_hp = tc.max_hp - tc.expected_damage;
        EXPECT_EQ(enemy.current_hp, expected_hp)
            << "Max HP " << tc.max_hp << " should lose " << tc.expected_damage << " HP to burn";
    }
}

TEST_F(BurnDamageTest, EndOfTurn_DoesNotOverkill) {
    // Setup: Pokemon with 5 HP, burn damage would be 12 HP
    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 100;
    defender.current_hp = 5;  // Less than burn damage (12)

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    engine.ExecuteTurn(player_action, enemy_action);

    // Verify: HP should be clamped at 0, not negative
    const battle::state::Pokemon& enemy = engine.GetEnemy();
    EXPECT_EQ(enemy.current_hp, 0) << "HP should be 0 (clamped, not negative)";
    EXPECT_TRUE(enemy.is_fainted) << "Pokemon should be marked as fainted";
}

TEST_F(BurnDamageTest, EndOfTurn_PersistsAcrossTurns) {
    // Setup: Burned Pokemon
    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    // Turn 1
    engine.ExecuteTurn(player_action, enemy_action);
    EXPECT_EQ(engine.GetEnemy().current_hp, 88) << "Turn 1: Should lose 12 HP";

    // Turn 2
    engine.ExecuteTurn(player_action, enemy_action);
    EXPECT_EQ(engine.GetEnemy().current_hp, 76) << "Turn 2: Should lose another 12 HP";

    // Turn 3
    engine.ExecuteTurn(player_action, enemy_action);
    EXPECT_EQ(engine.GetEnemy().current_hp, 64) << "Turn 3: Should lose another 12 HP";
}

TEST_F(BurnDamageTest, EndOfTurn_OnlyBurnedPokemonTakeDamage) {
    // Setup: Only enemy is burned
    attacker.status1 = 0;  // Not burned
    attacker.max_hp = 100;
    attacker.current_hp = 100;

    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    engine.ExecuteTurn(player_action, enemy_action);

    // Verify: Only enemy took burn damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 100) << "Non-burned Pokemon should not take damage";
    EXPECT_EQ(engine.GetEnemy().current_hp, 88) << "Burned Pokemon should take damage";
}

TEST_F(BurnDamageTest, EndOfTurn_BothPokemonCanBeBurned) {
    // Setup: Both Pokemon burned
    attacker.status1 = domain::Status1::BURN;
    attacker.max_hp = 80;
    attacker.current_hp = 80;

    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 100;
    defender.current_hp = 100;

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    engine.ExecuteTurn(player_action, enemy_action);

    // Verify: Both took burn damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 70) << "Player: 80 - 10 = 70 HP";
    EXPECT_EQ(engine.GetEnemy().current_hp, 88) << "Enemy: 100 - 12 = 88 HP";
}

TEST_F(BurnDamageTest, EndOfTurn_ExactlyDivides) {
    // Setup: Max HP = 8, so 8/8 = 1 damage (exact division)
    defender.status1 = domain::Status1::BURN;
    defender.max_hp = 8;
    defender.current_hp = 8;

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                       domain::Move::Growl};
    battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                      domain::Move::Growl};

    engine.ExecuteTurn(player_action, enemy_action);

    EXPECT_EQ(engine.GetEnemy().current_hp, 7) << "Should deal exactly 1 damage (8/8 = 1)";
}

// ============================================================================
// Attack Reduction Tests
// ============================================================================

TEST_F(BurnDamageTest, AttackReduction_ReducesAttackBy50Percent) {
    // Setup: Burned attacker with known attack stat
    attacker.status1 = domain::Status1::BURN;
    attacker.attack = 100;
    defender.defense = 50;

    domain::MoveData move = CreateTackle();
    move.power = 40;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Hit(ctx);

    // Expected damage with burn:
    // Normal attack would be 100
    // Burned attack is 100/2 = 50
    // damage = ((22 * 40 * 50 / 50) / 50) + 2 = ((44000 / 50) / 50) + 2 = 17 + 2 = 19
    // Without burn: ((22 * 40 * 100 / 50) / 50) + 2 = ((88000 / 50) / 50) + 2 = 35 + 2 = 37

    // Damage should be roughly half
    EXPECT_LT(ctx.damage_dealt, 25) << "Burn should reduce damage significantly";
    EXPECT_GT(ctx.damage_dealt, 15) << "Burn should still deal some damage";
}

TEST_F(BurnDamageTest, AttackReduction_WithPositiveStatStages) {
    // Setup: Burned Pokemon with +2 Attack stages
    attacker.status1 = domain::Status1::BURN;
    attacker.attack = 100;
    attacker.stat_stages[domain::STAT_ATK] = 2;  // +2 Attack
    defender.defense = 50;

    domain::MoveData move = CreateTackle();
    move.power = 40;

    battle::BattleContext ctx = CreateBattleContext(&attacker, &defender, &move);
    battle::effects::Effect_Hit(ctx);

    // Expected calculation:
    // Base attack: 100
    // With +2 stages: 100 * (2+2)/2 = 100 * 2 = 200
    // With burn: 200 / 2 = 100
    // So effective attack = 100 (same as non-burned, non-boosted)

    uint16_t damage_with_burn_and_boost = ctx.damage_dealt;

    // Compare to non-burned, non-boosted baseline
    battle::state::Pokemon baseline_attacker = CreateCharmander();
    baseline_attacker.attack = 100;
    baseline_attacker.status1 = 0;                        // Not burned
    baseline_attacker.stat_stages[domain::STAT_ATK] = 0;  // No boost

    battle::state::Pokemon baseline_defender = CreateBulbasaur();
    baseline_defender.defense = 50;

    battle::BattleContext baseline_ctx =
        CreateBattleContext(&baseline_attacker, &baseline_defender, &move);
    battle::effects::Effect_Hit(baseline_ctx);

    // Damage should be similar (burn cancels out the +2 boost)
    EXPECT_NEAR(damage_with_burn_and_boost, baseline_ctx.damage_dealt, 2)
        << "Burn (+2 stages) should approximately cancel to baseline damage";
}

TEST_F(BurnDamageTest, AttackReduction_OnlyAffectsAttackStat) {
    // Verify burn doesn't affect defense, speed, etc.
    attacker.status1 = domain::Status1::BURN;
    attacker.defense = 100;
    attacker.sp_attack = 100;
    attacker.sp_defense = 100;
    attacker.speed = 100;

    // Get modified stats
    int defense = battle::commands::GetModifiedStat(attacker, domain::STAT_DEF);
    int sp_attack = battle::commands::GetModifiedStat(attacker, domain::STAT_SPATK);
    int sp_defense = battle::commands::GetModifiedStat(attacker, domain::STAT_SPDEF);
    int speed = battle::commands::GetModifiedStat(attacker, domain::STAT_SPEED);

    // All should be unchanged
    EXPECT_EQ(defense, 100) << "Defense should not be affected by burn";
    EXPECT_EQ(sp_attack, 100) << "Special Attack should not be affected by burn";
    EXPECT_EQ(sp_defense, 100) << "Special Defense should not be affected by burn";
    EXPECT_EQ(speed, 100) << "Speed should not be affected by burn (paralysis does that)";
}

TEST_F(BurnDamageTest, AttackReduction_NonBurnedPokemonUnaffected) {
    // Verify non-burned Pokemon have normal attack
    attacker.status1 = 0;  // Not burned
    attacker.attack = 100;

    int attack = battle::commands::GetModifiedStat(attacker, domain::STAT_ATK);
    EXPECT_EQ(attack, 100) << "Non-burned Pokemon should have full attack";
}

// ============================================================================
// Integration Tests (Burn Application → Damage → End-of-Turn)
// ============================================================================

TEST_F(BurnDamageTest, Integration_BurnedPokemonTakesDamageOverMultipleTurns) {
    // Full integration: Manually apply burn, then verify damage-over-time across multiple turns
    battle::BattleEngine engine;

    // Setup: Create Pokemon and manually apply burn status
    battle::state::Pokemon attacker_poke = CreateCharmander();
    battle::state::Pokemon defender_poke = CreateBulbasaur();
    defender_poke.max_hp = 100;
    defender_poke.current_hp = 100;
    defender_poke.status1 = domain::Status1::BURN;  // Manually burn

    engine.InitBattle(attacker_poke, defender_poke);

    // Turn 1: Both use Growl (non-damaging) to isolate burn damage
    battle::BattleAction player_growl{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                      domain::Move::Growl};
    battle::BattleAction enemy_growl{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                     domain::Move::Growl};

    // Execute Turn 1
    engine.ExecuteTurn(player_growl, enemy_growl);

    // After turn 1: Should have taken 12 HP burn damage (100/8 = 12)
    EXPECT_EQ(engine.GetEnemy().current_hp, 88) << "Turn 1: Should lose 12 HP to burn (100/8)";
    EXPECT_EQ(engine.GetEnemy().status1, domain::Status1::BURN) << "Burn status should persist";

    // Execute Turn 2
    engine.ExecuteTurn(player_growl, enemy_growl);

    // After turn 2: Should have taken another 12 HP burn damage
    EXPECT_EQ(engine.GetEnemy().current_hp, 76)
        << "Turn 2: Should lose another 12 HP to burn (88 - 12 = 76)";
    EXPECT_EQ(engine.GetEnemy().status1, domain::Status1::BURN)
        << "Burn status should still persist";

    // Execute Turn 3
    engine.ExecuteTurn(player_growl, enemy_growl);

    // After turn 3: Should have taken another 12 HP burn damage
    EXPECT_EQ(engine.GetEnemy().current_hp, 64)
        << "Turn 3: Should lose another 12 HP to burn (76 - 12 = 64)";
}
