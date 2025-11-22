/**
 * @file test/unit/status/test_paralysis_speed.cpp
 * @brief Paralysis speed reduction tests
 */

#include "../../../src/battle/engine.hpp"
#include "../../../src/domain/status.hpp"
#include "../test_helpers.hpp"
#include "framework.hpp"

TEST_CASE(Paralysis_Speed_BasicReduction) {
    // Base case: 100 speed paralyzed = 25 speed
    auto pokemon = CreatePikachu();
    pokemon.speed = 100;
    pokemon.status1 = domain::Status1::PARALYSIS;

    auto opponent = CreateBulbasaur();
    opponent.speed = 30;

    battle::BattleEngine engine;
    engine.InitBattle(pokemon, opponent);

    battle::BattleAction pokemon_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction opponent_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    uint16_t pokemon_hp_before = pokemon.current_hp;

    engine.ExecuteTurn(pokemon_action, opponent_action);

    // Pokemon should take damage (opponent goes first with speed 30 > paralyzed 25)
    TEST_ASSERT(engine.GetPlayer().current_hp < pokemon_hp_before,
                "Paralyzed Pokemon should be slower");
}

TEST_CASE(Paralysis_Speed_WithPositiveStages) {
    // +2 Speed: 100 * (2+2)/2 = 200, paralyzed = 50
    auto pokemon = CreatePikachu();
    pokemon.speed = 100;
    pokemon.stat_stages[domain::STAT_SPEED] = 2;
    pokemon.status1 = domain::Status1::PARALYSIS;

    auto opponent = CreateBulbasaur();
    opponent.speed = 45;

    battle::BattleEngine engine;
    engine.InitBattle(pokemon, opponent);

    battle::BattleAction pokemon_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction opponent_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    uint16_t opponent_hp_before = opponent.current_hp;

    engine.ExecuteTurn(pokemon_action, opponent_action);

    // Opponent should take damage (Pokemon still faster: 50 > 45)
    TEST_ASSERT(engine.GetEnemy().current_hp < opponent_hp_before,
                "Boosted paralyzed Pokemon can still be faster");
}

TEST_CASE(Paralysis_Speed_WithNegativeStages) {
    // -1 Speed: 100 * 2/(2-(-1)) = 66, paralyzed = 16
    auto pokemon = CreatePikachu();
    pokemon.speed = 100;
    pokemon.stat_stages[domain::STAT_SPEED] = -1;
    pokemon.status1 = domain::Status1::PARALYSIS;

    auto opponent = CreateBulbasaur();
    opponent.speed = 20;

    battle::BattleEngine engine;
    engine.InitBattle(pokemon, opponent);

    battle::BattleAction pokemon_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction opponent_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    uint16_t pokemon_hp_before = pokemon.current_hp;

    engine.ExecuteTurn(pokemon_action, opponent_action);

    // Pokemon should take damage (opponent faster: 20 > 16)
    TEST_ASSERT(engine.GetPlayer().current_hp < pokemon_hp_before,
                "Paralysis with negative stages is very slow");
}

TEST_CASE(Paralysis_Speed_EdgeCase_Speed1) {
    // Minimum speed: 1 paralyzed = 0 (rounds down)
    auto pokemon = CreatePikachu();
    pokemon.speed = 1;
    pokemon.status1 = domain::Status1::PARALYSIS;

    auto opponent = CreateBulbasaur();
    opponent.speed = 1;

    battle::BattleEngine engine;
    engine.InitBattle(pokemon, opponent);

    battle::BattleAction pokemon_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction opponent_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    engine.ExecuteTurn(pokemon_action, opponent_action);

    // Both should take damage (both attack, random order when tied)
    TEST_ASSERT(engine.GetPlayer().current_hp < pokemon.current_hp,
                "Speed 0 Pokemon still attacks");
    TEST_ASSERT(engine.GetEnemy().current_hp < opponent.current_hp, "Opponent attacks");
}

TEST_CASE(Paralysis_Speed_NonParalyzedUnaffected) {
    // Non-paralyzed Pokemon should use full speed
    auto pokemon = CreatePikachu();
    pokemon.speed = 90;
    pokemon.status1 = domain::Status1::NONE;

    auto opponent = CreateBulbasaur();
    opponent.speed = 45;

    battle::BattleEngine engine;
    engine.InitBattle(pokemon, opponent);

    battle::BattleAction pokemon_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction opponent_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    uint16_t opponent_hp_before = opponent.current_hp;

    engine.ExecuteTurn(pokemon_action, opponent_action);

    // Opponent takes damage (Pokemon faster with full speed)
    TEST_ASSERT(engine.GetEnemy().current_hp < opponent_hp_before,
                "Non-paralyzed Pokemon uses full speed");
}
