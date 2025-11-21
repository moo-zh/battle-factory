/**
 * @file test/test_effect_speed_down.cpp
 * @brief Tests for Effect_SpeedDown (String Shot)
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

// Helper: Create test Charmander (fast)
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
    p.speed = 13;  // Fast
    p.status1 = 0;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create test Bulbasaur (slower)
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
    p.speed = 9;  // Slower
    p.status1 = 0;

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    return p;
}

// Helper: Create String Shot move
static MoveData CreateStringShot() {
    MoveData m;
    m.move = Move::StringShot;
    m.type = Type::Normal;  // TODO: Change to Type::Bug when implemented
    m.power = 0;            // Status move, no damage
    m.accuracy = 95;
    m.pp = 40;
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

TEST_CASE(Effect_SpeedDown_LowersSpeedStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -1,
                "String Shot should lower Speed by 1 stage");
}

TEST_CASE(Effect_SpeedDown_DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "String Shot should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "String Shot should not calculate damage");
}

TEST_CASE(Effect_SpeedDown_CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use String Shot 3 times: -1, -1, -1 = -3
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_SpeedDown(ctx);
    ctx.move_failed = false;
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -3, "String Shot should stack to -3");
}

TEST_CASE(Effect_SpeedDown_MinimumStageMinus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -6, "Speed stage should not go below -6");
}

TEST_CASE(Effect_SpeedDown_CanLowerFromPositiveStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();
    defender.stat_stages[STAT_SPEED] = +2;  // Boosted by Agility

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == +1, "String Shot should lower from +2 to +1");
}

TEST_CASE(Effect_SpeedDown_DoesNotModifyAttacker) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    int8_t original_stage = attacker.stat_stages[STAT_SPEED];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == original_stage,
                "String Shot should not affect attacker's stats");
}

TEST_CASE(Effect_SpeedDown_DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    // Only Speed should be lowered
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == -1, "Speed should be -1");
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_SpeedDown_DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateStringShot();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedDown(ctx);

    TEST_ASSERT(!defender.is_fainted, "String Shot should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}
