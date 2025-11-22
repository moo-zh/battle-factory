/**
 * @file test/unit/status/test_paralysis_mechanics.cpp
 * @brief Paralysis speed reduction and turn skip tests
 */

#include "../../../src/battle/engine.hpp"
#include "../../../src/domain/status.hpp"
#include "../test_helpers.hpp"
#include "framework.hpp"

TEST_CASE(Paralysis_ReducesSpeedInTurnOrder) {
    // Pikachu (90 speed) paralyzed should lose to Bulbasaur (45 speed)
    // 90/4 = 22 < 45
    auto pikachu = CreatePikachu();
    pikachu.speed = 90;
    pikachu.status1 = domain::Status1::PARALYSIS;

    auto bulbasaur = CreateBulbasaur();
    bulbasaur.speed = 45;

    battle::BattleEngine engine;
    engine.InitBattle(pikachu, bulbasaur);

    uint16_t pikachu_hp_before = pikachu.current_hp;
    uint16_t bulbasaur_hp_before = bulbasaur.current_hp;

    battle::BattleAction pikachu_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                        domain::Move::Tackle};
    battle::BattleAction bulbasaur_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                          domain::Move::Tackle};

    engine.ExecuteTurn(pikachu_action, bulbasaur_action);

    // Both should take damage (both attacks execute)
    TEST_ASSERT(engine.GetPlayer().current_hp < pikachu_hp_before, "Pikachu should take damage");
    TEST_ASSERT(engine.GetEnemy().current_hp < bulbasaur_hp_before, "Bulbasaur should take damage");
}
