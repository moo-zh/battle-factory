/**
 * @file test/unit/status/test_paralysis_immunity.cpp
 * @brief Paralysis type immunity tests
 */

#include "../../../src/battle/commands/status.hpp"
#include "../../../src/domain/status.hpp"
#include "../test_helpers.hpp"
#include "framework.hpp"

TEST_CASE(Paralysis_Immunity_ElectricType_Pure) {
    // Pure Electric type immune to Electric-type paralysis
    auto attacker = CreateBulbasaur();
    auto defender = CreatePikachu();  // Pure Electric

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::NONE, "Electric type immune to Thunder Wave");
}

TEST_CASE(Paralysis_Immunity_ElectricType_Dual) {
    // Electric/Flying type immune to Electric-type paralysis
    auto attacker = CreateBulbasaur();
    auto defender = CreatePikachu();
    defender.type2 = domain::Type::Flying;  // Make it Electric/Flying

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::NONE,
                "Electric/Flying immune to Thunder Wave");
}

TEST_CASE(Paralysis_Immunity_NonElectricNotImmune) {
    // Fire type not immune to Electric-type paralysis
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();  // Fire type

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::PARALYSIS,
                "Non-Electric type can be paralyzed");
}

TEST_CASE(Paralysis_Immunity_OnlyElectricMoves) {
    // Electric type CAN be paralyzed by non-Electric moves (like Body Slam)
    auto attacker = CreateBulbasaur();
    auto defender = CreatePikachu();  // Electric type

    // Simulate Body Slam (Normal-type move with paralysis chance)
    domain::MoveData body_slam = {domain::Move::Tackle,  // Using Tackle as placeholder
                                  domain::Type::Normal,  // Normal type, not Electric
                                  85,
                                  100,
                                  15,
                                  30,
                                  0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &body_slam;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::PARALYSIS,
                "Electric type CAN be paralyzed by Normal-type moves");
}

TEST_CASE(Paralysis_Immunity_AlreadyStatused) {
    // Already burned Pokemon cannot be paralyzed
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    defender.status1 = domain::Status1::BURN;

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::BURN,
                "Already statused Pokemon cannot be paralyzed");
}

TEST_CASE(Paralysis_Immunity_FaintedPokemon) {
    // Fainted Pokemon cannot be paralyzed
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    defender.current_hp = 0;
    defender.is_fainted = true;

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 100, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 100);

    TEST_ASSERT(defender.status1 == domain::Status1::NONE, "Fainted Pokemon cannot be paralyzed");
}

TEST_CASE(Paralysis_Immunity_ChanceParameter) {
    // 0% chance should never apply
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();

    domain::MoveData thunder_wave = {
        domain::Move::ThunderWave, domain::Type::Electric, 0, 100, 20, 0, 0};

    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;
    ctx.move = &thunder_wave;
    ctx.move_failed = false;

    battle::commands::TryApplyParalysis(ctx, 0);

    TEST_ASSERT(defender.status1 == domain::Status1::NONE, "0% chance never applies paralysis");
}
