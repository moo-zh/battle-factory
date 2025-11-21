/**
 * @file test/integration/test_engine_phase1.cpp
 * @brief Integration tests for BattleEngine Phase 1 (Walking Skeleton)
 *
 * Phase 1 tests validate:
 * - Engine initialization
 * - Basic turn execution (Tackle vs Tackle)
 * - Hardcoded turn order (player first)
 * - Battle over detection
 *
 * Success criteria (ADR-001):
 * - Both Pokemon take damage
 * - Damage values are reasonable
 * - No crashes or assertions
 * - Binary size increase < 5KB
 */

#include "../../source/battle/engine.hpp"
#include "../../source/battle/state/pokemon.hpp"
#include "../../source/domain/move.hpp"
#include "../../source/domain/species.hpp"
#include "../../source/domain/stats.hpp"
#include "../framework.hpp"

using namespace battle;
using namespace battle::state;
using namespace domain;

// Helper: Create test Charmander (player)
static Pokemon CreateCharmander() {
    Pokemon p;
    p.species = Species::Charmander;
    p.level = 5;
    p.type1 = Type::Fire;
    p.type2 = Type::None;
    p.max_hp = 50;  // Increased to survive multiple Tackles
    p.current_hp = 50;
    p.attack = 11;
    p.defense = 9;
    p.sp_attack = 12;
    p.sp_defense = 10;
    p.speed = 13;
    p.status1 = 0;
    p.is_fainted = false;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create test Bulbasaur (enemy)
static Pokemon CreateBulbasaur() {
    Pokemon p;
    p.species = Species::Bulbasaur;
    p.level = 5;
    p.type1 = Type::Grass;
    p.type2 = Type::Poison;
    p.max_hp = 50;  // Increased to survive multiple Tackles
    p.current_hp = 50;
    p.attack = 10;
    p.defense = 10;
    p.sp_attack = 12;
    p.sp_defense = 12;
    p.speed = 9;
    p.status1 = 0;
    p.is_fainted = false;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

/**
 * @brief Phase 1 Core Test: Tackle vs Tackle deals damage to both Pokemon
 *
 * This is the fundamental integration test - if this works, the Engine ↔ Effect
 * integration is proven functional.
 */
TEST_CASE(Engine_TackleVsTackle_BothPokemonTakeDamage) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    // Both use Tackle (move_slot 0)
    BattleAction player_action{ActionType::MOVE, Player::PLAYER, 0};
    BattleAction enemy_action{ActionType::MOVE, Player::ENEMY, 0};

    engine.ExecuteTurn(player_action, enemy_action);

    // Verify both Pokemon took damage
    TEST_ASSERT(engine.GetPlayer().current_hp < 50, "Player should take damage from enemy Tackle");
    TEST_ASSERT(engine.GetEnemy().current_hp < 50, "Enemy should take damage from player Tackle");
}

/**
 * @brief Verify damage values are reasonable (not 0, not overkill)
 */
TEST_CASE(Engine_TackleVsTackle_DamageIsReasonable) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};
    engine.ExecuteTurn(tackle, tackle);

    // Expected damage for level 5 Tackle: roughly 18-21 damage per hit
    uint16_t player_damage_taken = 50 - engine.GetPlayer().current_hp;
    uint16_t enemy_damage_taken = 50 - engine.GetEnemy().current_hp;

    TEST_ASSERT(player_damage_taken >= 10 && player_damage_taken <= 30,
                "Player damage should be 10-30");
    TEST_ASSERT(enemy_damage_taken >= 10 && enemy_damage_taken <= 30,
                "Enemy damage should be 10-30");
}

/**
 * @brief Verify player attacks first (hardcoded turn order in Phase 1)
 *
 * Set player to 1 HP, enemy to full HP. If player goes first, enemy takes damage.
 * If enemy went first, player would faint and enemy would take no damage.
 */
TEST_CASE(Engine_TackleVsTackle_PlayerGoesFirst) {
    BattleEngine engine;

    // Player has 1 HP (would faint if hit)
    auto player = CreateCharmander();
    player.current_hp = 1;

    auto enemy = CreateBulbasaur();

    engine.InitBattle(player, enemy);

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};
    engine.ExecuteTurn(tackle, tackle);

    // Player attacked first, so enemy took damage
    // Then player got hit and fainted
    TEST_ASSERT(engine.GetEnemy().current_hp < 50, "Enemy should take damage (player went first)");
    TEST_ASSERT(engine.GetPlayer().is_fainted, "Player should faint after taking hit");
}

/**
 * @brief Verify battle ends when one Pokemon faints
 */
TEST_CASE(Engine_TackleVsTackle_BattleEndsOnFaint) {
    BattleEngine engine;

    // Enemy has 1 HP (will faint from first Tackle)
    auto player = CreateCharmander();
    auto enemy = CreateBulbasaur();
    enemy.current_hp = 1;

    engine.InitBattle(player, enemy);

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};
    engine.ExecuteTurn(tackle, tackle);

    // Battle should be over
    TEST_ASSERT(engine.IsBattleOver(), "Battle should be over when enemy faints");
    TEST_ASSERT(engine.GetEnemy().is_fainted, "Enemy should be marked as fainted");

    // Player should NOT take damage (turn ended after enemy fainted)
    TEST_ASSERT(engine.GetPlayer().current_hp == 50, "Player should not take damage (battle over)");
}

/**
 * @brief Verify IsBattleOver returns false when both Pokemon are alive
 */
TEST_CASE(Engine_IsBattleOver_FalseWhenBothAlive) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    TEST_ASSERT(!engine.IsBattleOver(), "Battle should not be over at start");

    // Execute one turn
    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};
    engine.ExecuteTurn(tackle, tackle);

    // Both should still be alive (50 HP should survive one Tackle)
    TEST_ASSERT(!engine.IsBattleOver(), "Battle should not be over after one turn");
}

/**
 * @brief Verify InitBattle correctly sets up Pokemon state
 */
TEST_CASE(Engine_InitBattle_CopiesPokemonState) {
    BattleEngine engine;

    auto player = CreateCharmander();
    auto enemy = CreateBulbasaur();

    engine.InitBattle(player, enemy);

    // Verify player state copied correctly
    TEST_ASSERT(engine.GetPlayer().species == Species::Charmander, "Player species correct");
    TEST_ASSERT(engine.GetPlayer().max_hp == 50, "Player max HP correct");
    TEST_ASSERT(engine.GetPlayer().current_hp == 50, "Player current HP correct");

    // Verify enemy state copied correctly
    TEST_ASSERT(engine.GetEnemy().species == Species::Bulbasaur, "Enemy species correct");
    TEST_ASSERT(engine.GetEnemy().max_hp == 50, "Enemy max HP correct");
    TEST_ASSERT(engine.GetEnemy().current_hp == 50, "Enemy current HP correct");
}

/**
 * @brief Stress test: Multiple turns until one Pokemon faints
 */
TEST_CASE(Engine_MultipleTurns_EventuallyOneFaints) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};

    // Execute up to 20 turns (should be enough to KO one Pokemon)
    int turns = 0;
    while (!engine.IsBattleOver() && turns < 20) {
        engine.ExecuteTurn(tackle, tackle);
        turns++;
    }

    // Battle should have ended
    TEST_ASSERT(engine.IsBattleOver(), "Battle should end within 20 turns");
    TEST_ASSERT(turns < 20, "Battle should not take 20 turns");

    // At least one Pokemon should be fainted
    TEST_ASSERT(engine.GetPlayer().is_fainted || engine.GetEnemy().is_fainted,
                "At least one Pokemon should be fainted");
}

/**
 * @brief Edge case: Both Pokemon at 1 HP
 */
TEST_CASE(Engine_BothAt1HP_PlayerWins) {
    BattleEngine engine;

    auto player = CreateCharmander();
    player.current_hp = 1;

    auto enemy = CreateBulbasaur();
    enemy.current_hp = 1;

    engine.InitBattle(player, enemy);

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0};
    engine.ExecuteTurn(tackle, tackle);

    // Player goes first, KOs enemy, battle ends
    TEST_ASSERT(engine.IsBattleOver(), "Battle should be over");
    TEST_ASSERT(engine.GetEnemy().is_fainted, "Enemy should faint (hit first)");
    TEST_ASSERT(!engine.GetPlayer().is_fainted, "Player should survive (turn ended)");
}
