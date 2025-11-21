/**
 * @file test/test_effect_attack_down.cpp
 * @brief Tests for Effect_AttackDown (Growl)
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

// Helper: Create Growl move
static MoveData CreateGrowl() {
    MoveData m;
    m.move = Move::Growl;
    m.type = Type::Normal;
    m.power = 0;  // Status move, no damage
    m.accuracy = 100;
    m.pp = 40;
    m.effect_chance = 0;  // Not applicable for stat moves
    return m;
}

// Helper: Create Tackle move (for integration test)
static MoveData CreateTackle() {
    MoveData m;
    m.move = Move::Tackle;
    m.type = Type::Normal;
    m.power = 40;
    m.accuracy = 100;
    m.pp = 35;
    m.effect_chance = 0;
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

TEST_CASE(Effect_AttackDown_LowersAttackStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -1, "Growl should lower Attack by 1 stage");
}

TEST_CASE(Effect_AttackDown_DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Growl should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Growl should not calculate damage");
}

TEST_CASE(Effect_AttackDown_CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Growl 3 times
    Effect_AttackDown(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_AttackDown(ctx);
    ctx.move_failed = false;
    Effect_AttackDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -3, "Growl should stack to -3");
}

TEST_CASE(Effect_AttackDown_MinimumStageMinus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();
    defender.stat_stages[STAT_ATK] = -6;  // Already at minimum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -6, "Attack stage should not go below -6");
    // TODO: Check for "won't go lower" message once messages are implemented
}

TEST_CASE(Effect_AttackDown_CanLowerFromPositiveStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();
    defender.stat_stages[STAT_ATK] = 2;  // Boosted by Swords Dance

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == 1, "Growl should lower from +2 to +1");
}

TEST_CASE(Effect_AttackDown_DoesNotModifyAttacker) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();

    int8_t original_stage = attacker.stat_stages[STAT_ATK];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == original_stage,
                "Growl should not affect attacker's stats");
}

TEST_CASE(Effect_AttackDown_DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    // Only Attack should be lowered
    TEST_ASSERT(defender.stat_stages[STAT_ATK] == -1, "Attack should be -1");
    TEST_ASSERT(defender.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(defender.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_AttackDown_DoesNotCauseFaint) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateGrowl();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackDown(ctx);

    TEST_ASSERT(!defender.is_fainted, "Growl should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

// Integration test: Verify stat stages affect damage calculation
TEST_CASE(StatStages_AttackStageAffectsDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with -1 Attack
    attacker.stat_stages[STAT_ATK] = -1;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t reduced_damage = ctx2.damage_dealt;

    // With -1 Attack, damage should be reduced (multiplied by 2/3)
    TEST_ASSERT(reduced_damage < normal_damage, "Damage should be reduced with -1 Attack stage");

    // Allow 2 damage rounding error
    uint16_t expected_damage = (normal_damage * 2) / 3;
    int16_t diff = (int16_t)reduced_damage - (int16_t)expected_damage;
    if (diff < 0)
        diff = -diff;
    TEST_ASSERT(diff <= 2, "Reduced damage should be approximately 2/3 of normal damage");
}
