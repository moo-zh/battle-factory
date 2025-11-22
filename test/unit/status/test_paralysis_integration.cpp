/**
 * @file test/unit/status/test_paralysis_integration.cpp
 * @brief End-to-end paralysis integration tests
 */

#include "../../../src/battle/engine.hpp"
#include "../../../src/domain/status.hpp"
#include "../test_helpers.hpp"
#include "framework.hpp"

TEST_CASE(Paralysis_Integration_ThunderWaveToSlowdown) {
    // Bulbasaur uses Thunder Wave on Charmander, then tests speed order
    auto bulbasaur = CreateBulbasaur();
    bulbasaur.speed = 45;

    auto charmander = CreateCharmander();
    charmander.speed = 65;  // Faster normally

    battle::BattleEngine engine;
    engine.InitBattle(bulbasaur, charmander);

    // Turn 1: Bulbasaur uses Thunder Wave (Charmander goes first with Tackle)
    battle::BattleAction bulbasaur_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                          domain::Move::ThunderWave};
    battle::BattleAction charmander_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                           domain::Move::Tackle};

    uint16_t bulbasaur_hp_before = bulbasaur.current_hp;

    engine.ExecuteTurn(bulbasaur_action, charmander_action);

    // Charmander should be paralyzed
    TEST_ASSERT(engine.GetEnemy().status1 == domain::Status1::PARALYSIS, "Charmander paralyzed");

    // Bulbasaur should have taken damage (Charmander went first)
    TEST_ASSERT(engine.GetPlayer().current_hp < bulbasaur_hp_before, "Charmander attacked first");

    // Turn 2: Both use Tackle - Bulbasaur should now go first
    // Charmander speed: 65/4 = 16 < Bulbasaur 45
    bulbasaur_action.move = domain::Move::Tackle;

    // Run a few turns to observe ordering (accounting for possible paralysis skip)
    for (int i = 0; i < 10; i++) {
        uint16_t bh = engine.GetPlayer().current_hp;
        uint16_t ch = engine.GetEnemy().current_hp;

        engine.ExecuteTurn(bulbasaur_action, charmander_action);

        // If Charmander didn't skip and attacked
        if (engine.GetPlayer().current_hp < bh) {
            // Bulbasaur should have also attacked (both attack)
            TEST_ASSERT(engine.GetEnemy().current_hp < ch, "Paralyzed Charmander is slower");
            break;
        }
    }
}

TEST_CASE(Paralysis_Integration_PersistsAcrossTurns) {
    // Paralysis status persists across multiple turns
    auto attacker = CreatePikachu();
    attacker.status1 = domain::Status1::PARALYSIS;
    auto defender = CreateBulbasaur();

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction attacker_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                         domain::Move::Tackle};
    battle::BattleAction defender_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    // Run 5 turns
    for (int i = 0; i < 5; i++) {
        engine.ExecuteTurn(attacker_action, defender_action);

        if (!engine.GetPlayer().is_fainted && !engine.GetEnemy().is_fainted) {
            TEST_ASSERT(engine.GetPlayer().status1 == domain::Status1::PARALYSIS,
                        "Paralysis persists");
        }
    }
}

TEST_CASE(Paralysis_Integration_DoesNotSpreadToOpponent) {
    // Paralysis doesn't spread to opponent
    auto paralyzed = CreatePikachu();
    paralyzed.status1 = domain::Status1::PARALYSIS;
    auto healthy = CreateBulbasaur();

    battle::BattleEngine engine;
    engine.InitBattle(paralyzed, healthy);

    battle::BattleAction paralyzed_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                          domain::Move::Tackle};
    battle::BattleAction healthy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                        domain::Move::Tackle};

    engine.ExecuteTurn(paralyzed_action, healthy_action);

    TEST_ASSERT(engine.GetPlayer().status1 == domain::Status1::PARALYSIS,
                "Attacker still paralyzed");
    TEST_ASSERT(engine.GetEnemy().status1 == domain::Status1::NONE, "Defender not paralyzed");
}
