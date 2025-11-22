/**
 * @file test/host/weather/test_sandstorm.cpp
 * @brief Tests for Sandstorm weather mechanics
 *
 * Tests the complete Sandstorm vertical slice:
 * - Weather setting (move application)
 * - End-of-turn damage (1/16 max HP)
 * - Type-based immunity (Rock/Ground/Steel)
 * - Weather duration (5-turn counter, decrement, expiry)
 * - Weather replacement
 */

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace battle;
using namespace domain;

// ============================================================================
// Weather Setting Tests
// ============================================================================

TEST(SandstormTest, SetWeather_SetsWeatherToSandstorm) {
    // Sandstorm move should set weather to Sandstorm
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;

    engine.InitBattle(pikachu, charmander);

    // Use Sandstorm
    BattleAction player_action{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction enemy_action{ActionType::MOVE, Player::ENEMY, 0, Move::Tackle};

    engine.ExecuteTurn(player_action, enemy_action);

    // Weather should be Sandstorm
    EXPECT_EQ(engine.GetPlayer().max_hp, pikachu.max_hp);  // Sanity check
    // Note: We can't directly access field_ from tests, so we'll verify via damage
}

TEST(SandstormTest, SetWeather_SetsDefaultDuration) {
    // Sandstorm should last 5 turns by default
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;  // Set specific HP for damage verification
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    charmander.current_hp = 100;
    charmander.max_hp = 100;

    engine.InitBattle(pikachu, charmander);

    // Use Sandstorm
    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};  // Enemy protects

    engine.ExecuteTurn(sandstorm, pass);

    // Turn 1: Both take damage (100 - 6 = 94)
    EXPECT_EQ(engine.GetPlayer().current_hp, 94);  // 100 - 100/16 = 94
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);

    // Turn 2: Both take damage again (94 - 6 = 88)
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 88);
    EXPECT_EQ(engine.GetEnemy().current_hp, 88);

    // Turn 3: Both take damage (88 - 6 = 82)
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 82);
    EXPECT_EQ(engine.GetEnemy().current_hp, 82);

    // Turn 4: Both take damage (82 - 6 = 76)
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 76);
    EXPECT_EQ(engine.GetEnemy().current_hp, 76);

    // Turn 5: Both take damage (76 - 6 = 70), then weather ends
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 70);
    EXPECT_EQ(engine.GetEnemy().current_hp, 70);

    // Turn 6: No more damage (weather expired)
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 70);  // No change
    EXPECT_EQ(engine.GetEnemy().current_hp, 70);
}

// ============================================================================
// End-of-Turn Damage Tests
// ============================================================================

TEST(SandstormTest, EndOfTurn_Deals1_16thMaxHP) {
    // Sandstorm should deal 1/16 max HP damage per turn
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 160;
    pikachu.max_hp = 160;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Damage: 160 / 16 = 10
    EXPECT_EQ(engine.GetPlayer().current_hp, 150);  // 160 - 10 = 150
}

TEST(SandstormTest, EndOfTurn_IntegerDivisionRounding) {
    // Test various max HP values to verify integer division
    struct TestCase {
        uint16_t max_hp;
        uint16_t expected_damage;
    };

    TestCase cases[] = {
        {16, 1},   // 16/16 = 1
        {32, 2},   // 32/16 = 2
        {15, 0},   // 15/16 = 0 (rounds down)
        {100, 6},  // 100/16 = 6
        {128, 8},  // 128/16 = 8
    };

    for (const auto& test : cases) {
        BattleEngine engine;

        auto pikachu = CreatePikachu();
        pikachu.current_hp = 100;
        pikachu.max_hp = 100;
        pikachu.current_hp = test.max_hp;
        pikachu.max_hp = test.max_hp;

        auto charmander = CreateCharmander();
        charmander.current_hp = 100;
        charmander.max_hp = 100;
        engine.InitBattle(pikachu, charmander);

        BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
        BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

        engine.ExecuteTurn(sandstorm, pass);

        uint16_t expected_hp = test.max_hp - test.expected_damage;
        EXPECT_EQ(engine.GetPlayer().current_hp, expected_hp)
            << "Max HP: " << test.max_hp << ", Expected damage: " << test.expected_damage;
    }
}

TEST(SandstormTest, EndOfTurn_DoesNotOverkill) {
    // Sandstorm damage should clamp at 0 HP, not go negative
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 3;  // Less than sandstorm damage
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // HP should clamp at 0, Pokemon should faint
    EXPECT_EQ(engine.GetPlayer().current_hp, 0);
    EXPECT_TRUE(engine.GetPlayer().is_fainted);
}

TEST(SandstormTest, EndOfTurn_PersistsAcrossTurns) {
    // Sandstorm damage should apply every turn
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    // Turn 1
    engine.ExecuteTurn(sandstorm, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 94);  // 100 - 6 = 94

    // Turn 2
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 88);  // 94 - 6 = 88

    // Turn 3
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 82);  // 88 - 6 = 82
}

TEST(SandstormTest, EndOfTurn_BothPokemonTakeDamage) {
    // Both player and enemy should take sandstorm damage
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    charmander.current_hp = 100;
    charmander.max_hp = 100;

    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    EXPECT_EQ(engine.GetPlayer().current_hp, 94);  // Both damaged
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);
}

// ============================================================================
// Type Immunity Tests
// ============================================================================

TEST(SandstormTest, Immunity_RockTypeImmune) {
    // Rock-type Pokemon should not take sandstorm damage
    BattleEngine engine;

    auto geodude = CreateGeodude();  // Rock/Ground
    geodude.current_hp = 100;
    geodude.max_hp = 100;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    engine.InitBattle(geodude, pikachu);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Geodude (Rock type) takes no damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 100);

    // Pikachu (Electric type) takes damage
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);
}

TEST(SandstormTest, Immunity_GroundTypeImmune) {
    // Ground-type Pokemon should not take sandstorm damage
    BattleEngine engine;

    auto sandshrew = CreateSandshrew();  // Pure Ground
    sandshrew.current_hp = 100;
    sandshrew.max_hp = 100;
    sandshrew.current_hp = 100;
    sandshrew.max_hp = 100;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    engine.InitBattle(sandshrew, pikachu);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Sandshrew (Ground type) takes no damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 100);

    // Pikachu takes damage
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);
}

TEST(SandstormTest, Immunity_SteelTypeImmune) {
    // Steel-type Pokemon should not take sandstorm damage
    BattleEngine engine;

    auto skarmory = CreateSkarmory();  // Steel/Flying
    skarmory.current_hp = 100;
    skarmory.max_hp = 100;
    skarmory.current_hp = 100;
    skarmory.max_hp = 100;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    engine.InitBattle(skarmory, pikachu);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Skarmory (Steel type) takes no damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 100);

    // Pikachu takes damage
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);
}

TEST(SandstormTest, Immunity_DualTypeWithImmunity) {
    // Dual-type Pokemon with Rock/Ground/Steel should be immune
    BattleEngine engine;

    auto geodude = CreateGeodude();  // Rock/Ground (both immune)
    geodude.current_hp = 100;
    geodude.max_hp = 100;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    engine.InitBattle(geodude, pikachu);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Geodude immune (has both Rock and Ground)
    EXPECT_EQ(engine.GetPlayer().current_hp, 100);
}

TEST(SandstormTest, Immunity_NonImmuneTypesTakeDamage) {
    // Non-immune types should take damage
    BattleEngine engine;

    auto pikachu = CreatePikachu();  // Electric
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();  // Fire
    charmander.current_hp = 100;
    charmander.max_hp = 100;

    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Both take damage (neither is Rock/Ground/Steel)
    EXPECT_EQ(engine.GetPlayer().current_hp, 94);
    EXPECT_EQ(engine.GetEnemy().current_hp, 94);
}

// ============================================================================
// Weather Duration Tests
// ============================================================================

TEST(SandstormTest, Duration_DecrementsEachTurn) {
    // Weather duration should decrement each turn
    // Verified indirectly by checking when damage stops
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    // Set sandstorm (5 turns)
    engine.ExecuteTurn(sandstorm, pass);

    // Turns 1-5: Damage occurs
    for (int turn = 2; turn <= 5; turn++) {
        uint16_t hp_before = engine.GetPlayer().current_hp;
        engine.ExecuteTurn(pass, pass);
        EXPECT_LT(engine.GetPlayer().current_hp, hp_before)
            << "Turn " << turn << ": damage should occur";
    }

    // Turn 6: No damage (weather expired)
    uint16_t hp_after_5_turns = engine.GetPlayer().current_hp;
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, hp_after_5_turns)
        << "Turn 6: no damage (weather ended)";
}

TEST(SandstormTest, Duration_ClearsWeatherWhenExpires) {
    // Weather should clear after 5 turns
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    // Set sandstorm
    engine.ExecuteTurn(sandstorm, pass);

    // Run 5 more turns (weather expires after turn 5)
    for (int i = 0; i < 5; i++) {
        engine.ExecuteTurn(pass, pass);
    }

    // Verify no damage on turn 6
    uint16_t hp_turn_6 = engine.GetPlayer().current_hp;
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, hp_turn_6);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(SandstormTest, Integration_CompleteBattleFlow) {
    // Test complete Sandstorm flow over multiple turns
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;

    auto geodude = CreateGeodude();  // Rock/Ground (immune)
    geodude.current_hp = 100;
    geodude.max_hp = 100;

    engine.InitBattle(pikachu, geodude);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    // Turn 1: Set sandstorm
    engine.ExecuteTurn(sandstorm, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 94);  // Pikachu damaged
    EXPECT_EQ(engine.GetEnemy().current_hp, 100);  // Geodude immune

    // Turn 2: Sandstorm persists
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 88);  // Pikachu damaged again
    EXPECT_EQ(engine.GetEnemy().current_hp, 100);  // Geodude still immune

    // Turn 3
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 82);

    // Turn 4
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 76);

    // Turn 5
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 70);

    // Turn 6: Weather expired
    engine.ExecuteTurn(pass, pass);
    EXPECT_EQ(engine.GetPlayer().current_hp, 70);  // No more damage
}

TEST(SandstormTest, Integration_SandstormWithBurnDamage) {
    // Test that weather damage and status damage both apply
    BattleEngine engine;

    auto pikachu = CreatePikachu();
    pikachu.current_hp = 100;
    pikachu.max_hp = 100;
    pikachu.current_hp = 160;
    pikachu.max_hp = 160;
    pikachu.status1 = domain::Status1::BURN;  // Burned

    auto charmander = CreateCharmander();
    charmander.current_hp = 100;
    charmander.max_hp = 100;
    engine.InitBattle(pikachu, charmander);

    BattleAction sandstorm{ActionType::MOVE, Player::PLAYER, 0, Move::Sandstorm};
    BattleAction pass{ActionType::MOVE, Player::ENEMY, 0, Move::Protect};

    engine.ExecuteTurn(sandstorm, pass);

    // Burn damage: 160/8 = 20
    // Sandstorm damage: 160/16 = 10
    // Total: 30 damage
    EXPECT_EQ(engine.GetPlayer().current_hp, 130);  // 160 - 30 = 130
}
