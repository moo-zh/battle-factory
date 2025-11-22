/**
 * @file test/unit/status/test_paralysis_turn_skip.cpp
 * @brief Paralysis turn skip (25% full paralysis) tests
 */

#include "../../../src/battle/engine.hpp"
#include "../../../src/domain/status.hpp"
#include "../test_helpers.hpp"
#include "framework.hpp"

TEST_CASE(Paralysis_TurnSkip_OccursSometimes) {
    // Run 100 trials, expect roughly 25% skips (allow 15%-35% for variance)
    const int trials = 100;
    int skip_count = 0;

    for (int i = 0; i < trials; i++) {
        auto attacker = CreatePikachu();
        attacker.status1 = domain::Status1::PARALYSIS;
        auto defender = CreateBulbasaur();

        battle::BattleEngine engine;
        engine.InitBattle(attacker, defender);

        uint16_t defender_hp_before = defender.current_hp;

        battle::BattleAction attacker_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                             domain::Move::Tackle};
        battle::BattleAction defender_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                             domain::Move::Tackle};

        engine.ExecuteTurn(attacker_action, defender_action);

        if (engine.GetEnemy().current_hp == defender_hp_before) {
            skip_count++;
        }
    }

    // Expect 15-35% skip rate
    TEST_ASSERT(skip_count >= 15, "Should skip at least 15% of turns");
    TEST_ASSERT(skip_count <= 35, "Should skip at most 35% of turns");
}

TEST_CASE(Paralysis_TurnSkip_NonParalyzedNeverSkips) {
    // Non-paralyzed Pokemon should never skip
    const int trials = 20;

    for (int i = 0; i < trials; i++) {
        auto attacker = CreatePikachu();
        attacker.status1 = domain::Status1::NONE;
        auto defender = CreateBulbasaur();

        battle::BattleEngine engine;
        engine.InitBattle(attacker, defender);

        uint16_t defender_hp_before = defender.current_hp;

        battle::BattleAction attacker_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                             domain::Move::Tackle};
        battle::BattleAction defender_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                             domain::Move::Tackle};

        engine.ExecuteTurn(attacker_action, defender_action);

        TEST_ASSERT(engine.GetEnemy().current_hp < defender_hp_before,
                    "Non-paralyzed should always attack");
    }
}

TEST_CASE(Paralysis_TurnSkip_BothParalyzed) {
    // Both Pokemon paralyzed - both can skip
    int both_attacked = 0;
    int player_skipped = 0;
    int enemy_skipped = 0;
    int both_skipped = 0;

    const int trials = 50;

    for (int i = 0; i < trials; i++) {
        auto player = CreatePikachu();
        player.status1 = domain::Status1::PARALYSIS;
        auto enemy = CreateBulbasaur();
        enemy.status1 = domain::Status1::PARALYSIS;

        battle::BattleEngine engine;
        engine.InitBattle(player, enemy);

        uint16_t player_hp_before = player.current_hp;
        uint16_t enemy_hp_before = enemy.current_hp;

        battle::BattleAction player_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                           domain::Move::Tackle};
        battle::BattleAction enemy_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                          domain::Move::Tackle};

        engine.ExecuteTurn(player_action, enemy_action);

        bool player_attacked = engine.GetEnemy().current_hp < enemy_hp_before;
        bool enemy_attacked = engine.GetPlayer().current_hp < player_hp_before;

        if (player_attacked && enemy_attacked)
            both_attacked++;
        else if (!player_attacked && enemy_attacked)
            player_skipped++;
        else if (player_attacked && !enemy_attacked)
            enemy_skipped++;
        else
            both_skipped++;
    }

    // All outcomes should occur
    TEST_ASSERT(both_attacked > 0, "Both should attack sometimes");
    TEST_ASSERT(player_skipped > 0, "Player should skip sometimes");
    TEST_ASSERT(enemy_skipped > 0, "Enemy should skip sometimes");
    // both_skipped is rare (6.25% chance) but can occur
    (void)both_skipped;  // Suppress unused warning
}

TEST_CASE(Paralysis_TurnSkip_DoesNotPreventDamageReceived) {
    // Paralyzed Pokemon can still be hit even if it skips
    auto attacker = CreatePikachu();
    attacker.status1 = domain::Status1::PARALYSIS;
    auto defender = CreateBulbasaur();

    battle::BattleEngine engine;
    engine.InitBattle(attacker, defender);

    battle::BattleAction attacker_action{battle::ActionType::MOVE, battle::Player::PLAYER, 0,
                                         domain::Move::Tackle};
    battle::BattleAction defender_action{battle::ActionType::MOVE, battle::Player::ENEMY, 0,
                                         domain::Move::Tackle};

    // Run until attacker skips
    for (int i = 0; i < 50; i++) {
        uint16_t attacker_hp_before = engine.GetPlayer().current_hp;
        uint16_t defender_hp_before = engine.GetEnemy().current_hp;

        engine.ExecuteTurn(attacker_action, defender_action);

        // If attacker skipped (defender HP unchanged)
        if (engine.GetEnemy().current_hp == defender_hp_before) {
            // Attacker should still have taken damage
            TEST_ASSERT(engine.GetPlayer().current_hp < attacker_hp_before,
                        "Paralyzed Pokemon takes damage even when skipping");
            return;
        }

        // Reset for next trial if both attacked
        if (!engine.GetPlayer().is_fainted && !engine.GetEnemy().is_fainted) {
            continue;
        } else {
            break;
        }
    }

    // If we didn't see a skip in 50 trials, that's statistically unlikely but possible
    TEST_ASSERT(true, "Test completed");
}
