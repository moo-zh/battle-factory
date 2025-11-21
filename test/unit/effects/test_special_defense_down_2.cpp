/**
 * @file test/unit/effects/test_special_defense_down_2.cpp
 * @brief Tests for Effect_SpecialDefenseDown2 (Fake Tears)
 */

#include "../../../source/battle/effects/basic.hpp"
#include "../../../source/battle/state/pokemon.hpp"
#include "../../../source/domain/move.hpp"
#include "../../../source/domain/species.hpp"
#include "../../../source/domain/stats.hpp"
#include "framework.hpp"

using namespace battle;
using namespace battle::effects;
using namespace battle::state;
using namespace domain;

// Helper: Create test Charmander
static Pokemon CreateCharmander() {
    Pokemon p;
    p.species = Species::Charmander;
    p.level = 5;
    p.type1 = Type::Fire;
    p.type2 = Type::None;
    p.max_hp = 20;
    p.current_hp = 20;
    p.attack = 11;
    p.defense = 9;
    p.sp_attack = 12;
    p.sp_defense = 10;
    p.speed = 13;
    p.status1 = 0;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create test Bulbasaur
static Pokemon CreateBulbasaur() {
    Pokemon p;
    p.species = Species::Bulbasaur;
    p.level = 5;
    p.type1 = Type::Grass;
    p.type2 = Type::Poison;
    p.max_hp = 21;
    p.current_hp = 21;
    p.attack = 10;
    p.defense = 10;
    p.sp_attack = 12;
    p.sp_defense = 12;
    p.speed = 9;
    p.status1 = 0;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create Fake Tears move
static MoveData CreateFakeTears() {
    MoveData m;
    m.move = Move::FakeTears;
    m.type = Type::Dark;
    m.power = 0;       // Status move, no damage
    m.accuracy = 100;  // Affects opponent, must check accuracy
    m.pp = 20;
    m.effect_chance = 0;  // Not applicable for stat moves
    return m;
}

// Helper: Set up battle context
static BattleContext SetupContext(Pokemon* attacker, Pokemon* defender, MoveData* move) {
    BattleContext ctx;
    ctx.attacker = attacker;
    ctx.defender = defender;
    ctx.move = move;
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // 1x (normal effectiveness)
    ctx.override_power = 0;
    ctx.override_type = 0;
    return ctx;
}

TEST_CASE(Effect_SpecialDefenseDown2_LowersSpecialDefenseStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -2,
                "Fake Tears should lower target's Sp. Defense by 2 stages");
}

TEST_CASE(Effect_SpecialDefenseDown2_DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Fake Tears should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Fake Tears should not calculate damage");
}

TEST_CASE(Effect_SpecialDefenseDown2_CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Fake Tears 3 times: -2, -2, -2 = -6 (capped)
    Effect_SpecialDefenseDown2(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_SpecialDefenseDown2(ctx);
    ctx.move_failed = false;
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -6, "Fake Tears should stack to -6 (cap)");
}

TEST_CASE(Effect_SpecialDefenseDown2_MinimumStageMinus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();
    defender.stat_stages[STAT_SPDEF] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -6, "Sp. Defense stage should not go below -6");
}

TEST_CASE(Effect_SpecialDefenseDown2_CapsAtMinus6FromMinus5) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();
    defender.stat_stages[STAT_SPDEF] = -5;  // One away from minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -6,
                "Fake Tears should cap at -6 (only -1 effective from -5)");
}

TEST_CASE(Effect_SpecialDefenseDown2_CanLowerFromPositiveStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();
    defender.stat_stages[STAT_SPDEF] = +2;  // Raised by Amnesia

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0, "Fake Tears should lower from +2 to 0");
}

TEST_CASE(Effect_SpecialDefenseDown2_DoesNotModifyAttacker) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();

    int8_t original_stage = attacker.stat_stages[STAT_SPDEF];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPDEF] == original_stage,
                "Fake Tears should not affect attacker's stats");
}

TEST_CASE(Effect_SpecialDefenseDown2_DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    // Only Sp. Defense should be lowered
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == -2, "Sp. Defense should be -2");
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
}

TEST_CASE(Effect_SpecialDefenseDown2_DoesNotCauseFaint) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(!defender.is_fainted, "Fake Tears should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

TEST_CASE(Effect_SpecialDefenseDown2_RequiresAccuracyCheck) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateFakeTears();
    move.accuracy = 100;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    ctx.move_failed = true;  // Simulate a miss

    // Since move failed, stat should not change
    Effect_SpecialDefenseDown2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0,
                "Fake Tears should not lower stats if move missed");
}
