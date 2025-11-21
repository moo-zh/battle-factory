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
    BattleAction player_action{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    BattleAction enemy_action{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};

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

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
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

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
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

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
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
    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
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

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};

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

    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    engine.ExecuteTurn(tackle, tackle);

    // Player goes first, KOs enemy, battle ends
    TEST_ASSERT(engine.IsBattleOver(), "Battle should be over");
    TEST_ASSERT(engine.GetEnemy().is_fainted, "Enemy should faint (hit first)");
    TEST_ASSERT(!engine.GetPlayer().is_fainted, "Player should survive (turn ended)");
}

// ============================================================================
// PHASE 2 TESTS: Thunder Wave (Status-only move)
// ============================================================================

/**
 * @brief Phase 2: Thunder Wave paralyzes enemy, enemy still attacks
 */
TEST_CASE(Engine_ThunderWaveVsTackle_BothExecute) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    // Player uses Thunder Wave, enemy uses Tackle
    BattleAction player_thunderwave{ActionType::MOVE, Player::PLAYER, 1, Move::ThunderWave};
    BattleAction enemy_tackle{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};

    engine.ExecuteTurn(player_thunderwave, enemy_tackle);

    // Player should be paralyzed (no damage from Thunder Wave)
    TEST_ASSERT(engine.GetPlayer().current_hp < 50, "Player should take damage from Tackle");

    // Enemy should be paralyzed (from Thunder Wave)
    TEST_ASSERT(engine.GetEnemy().status1 != 0, "Enemy should be paralyzed from Thunder Wave");

    // Enemy HP should not change (Thunder Wave deals no damage)
    TEST_ASSERT(engine.GetEnemy().current_hp == 50, "Thunder Wave should deal no damage");
}

/**
 * @brief Phase 2: Status-only move doesn't end battle
 */
TEST_CASE(Engine_ThunderWave_DoesNotEndBattle) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    // Both use Thunder Wave
    BattleAction thunderwave{ActionType::MOVE, Player::PLAYER, 1, Move::ThunderWave};

    engine.ExecuteTurn(thunderwave, thunderwave);

    // Battle should not be over (no damage dealt)
    TEST_ASSERT(!engine.IsBattleOver(), "Battle should not end from status moves");
    TEST_ASSERT(engine.GetPlayer().current_hp == 50, "Player HP unchanged");
    TEST_ASSERT(engine.GetEnemy().current_hp == 50, "Enemy HP unchanged");

    // Both should be paralyzed
    TEST_ASSERT(engine.GetPlayer().status1 != 0, "Player should be paralyzed");
    TEST_ASSERT(engine.GetEnemy().status1 != 0, "Enemy should be paralyzed");
}

/**
 * @brief Phase 2: Damage move followed by status move
 */
TEST_CASE(Engine_TackleFollowedByThunderWave) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    // Turn 1: Both Tackle
    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    engine.ExecuteTurn(tackle, tackle);

    uint16_t player_hp_after_turn1 = engine.GetPlayer().current_hp;
    uint16_t enemy_hp_after_turn1 = engine.GetEnemy().current_hp;

    // Turn 2: Player uses Thunder Wave, enemy uses Tackle
    BattleAction player_twave{ActionType::MOVE, Player::PLAYER, 1, Move::ThunderWave};
    BattleAction enemy_tackle{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};
    engine.ExecuteTurn(player_twave, enemy_tackle);

    // Player should take more damage from second Tackle
    TEST_ASSERT(engine.GetPlayer().current_hp < player_hp_after_turn1,
                "Player should take damage from second Tackle");

    // Enemy should not take damage from Thunder Wave
    TEST_ASSERT(engine.GetEnemy().current_hp == enemy_hp_after_turn1,
                "Enemy HP unchanged from Thunder Wave");

    // Enemy should be paralyzed
    TEST_ASSERT(engine.GetEnemy().status1 != 0, "Enemy should be paralyzed");
}

/**
 * @brief Phase 2: Both moves supported (Tackle and Thunder Wave)
 */
TEST_CASE(Engine_Phase2_SupportsTwoMoves) {
    BattleEngine engine;
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());

    // Verify Tackle still works
    BattleAction tackle{ActionType::MOVE, Player::PLAYER, 0, Move::Tackle};
    engine.ExecuteTurn(tackle, tackle);
    TEST_ASSERT(engine.GetEnemy().current_hp < 50, "Tackle should deal damage");

    // Reset and verify Thunder Wave works
    engine.InitBattle(CreateCharmander(), CreateBulbasaur());
    BattleAction twave{ActionType::MOVE, Player::PLAYER, 1, Move::ThunderWave};
    engine.ExecuteTurn(twave, twave);
    TEST_ASSERT(engine.GetEnemy().status1 != 0, "Thunder Wave should paralyze");
}
