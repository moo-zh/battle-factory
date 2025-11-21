/**
 * @file test/test_effect_speed_up_2.cpp
 * @brief Tests for Effect_SpeedUp2 (Agility)
 */

#include "../source/battle/effects/basic.hpp"
#include "../source/battle/state/pokemon.hpp"
#include "../source/domain/move.hpp"
#include "../source/domain/species.hpp"
#include "../source/domain/stats.hpp"
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

// Helper: Create Agility move
static MoveData CreateAgility() {
    MoveData m;
    m.move = Move::Agility;
    m.type = Type::Normal;  // TODO: Change to Type::Psychic when implemented
    m.power = 0;            // Status move, no damage
    m.accuracy = 0;         // Self-targeting moves can't miss (no accuracy check)
    m.pp = 30;
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

TEST_CASE(Effect_SpeedUp2_RaisesSpeedStage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +2,
                "Agility should raise user's Speed by 2 stages");
}

TEST_CASE(Effect_SpeedUp2_DoesNotDealDamage) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Agility should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Agility should not calculate damage");
}

TEST_CASE(Effect_SpeedUp2_CanStackMultipleTimes) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Agility 3 times: +2, +2, +2 = +6 (capped)
    Effect_SpeedUp2(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_SpeedUp2(ctx);
    ctx.move_failed = false;
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +6, "Agility should stack to +6 (cap)");
}

TEST_CASE(Effect_SpeedUp2_MaximumStagePlus6) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    attacker.stat_stages[STAT_SPEED] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +6, "Speed stage should not go above +6");
}

TEST_CASE(Effect_SpeedUp2_CapsAtPlus6FromPlus5) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    attacker.stat_stages[STAT_SPEED] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +6,
                "Agility should cap at +6 (only +1 effective from +5)");
}

TEST_CASE(Effect_SpeedUp2_CanRaiseFromNegativeStages) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    attacker.stat_stages[STAT_SPEED] = -2;  // Lowered by String Shot 2x

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == 0, "Agility should raise from -2 to 0");
}

TEST_CASE(Effect_SpeedUp2_DoesNotModifyDefender) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    int8_t original_stage = defender.stat_stages[STAT_SPEED];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == original_stage,
                "Agility should not affect defender's stats");
}

TEST_CASE(Effect_SpeedUp2_DoesNotAffectOtherStats) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    // Only Speed should be raised
    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +2, "Speed should be +2");
    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == 0, "Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_SpeedUp2_DoesNotCauseFaint) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    TEST_ASSERT(!defender.is_fainted, "Agility should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

TEST_CASE(Effect_SpeedUp2_NoAccuracyCheck) {
    auto attacker = CreateBulbasaur();
    auto defender = CreateCharmander();
    auto move = CreateAgility();
    move.accuracy = 0;  // Self-targeting moves have accuracy = 0

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_SpeedUp2(ctx);

    // Move should never miss
    TEST_ASSERT(!ctx.move_failed, "Agility should never miss (self-targeting)");
    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == +2,
                "Speed should be raised even with accuracy=0");
}
