/**
 * @file test/test_effect_attack_up_2.cpp
 * @brief Tests for Effect_AttackUp2 (Swords Dance)
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

// Helper: Create Swords Dance move
static MoveData CreateSwordsDance() {
    MoveData m;
    m.move = Move::SwordsDance;
    m.type = Type::Normal;
    m.power = 0;     // Status move, no damage
    m.accuracy = 0;  // Self-targeting moves can't miss (no accuracy check)
    m.pp = 30;
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

TEST_CASE(Effect_AttackUp2_RaisesAttackStage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == +2,
                "Swords Dance should raise user's Attack by 2 stages");
}

TEST_CASE(Effect_AttackUp2_DoesNotDealDamage) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    uint16_t original_hp = defender.current_hp;

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(defender.current_hp == original_hp, "Swords Dance should not deal damage");
    TEST_ASSERT(ctx.damage_dealt == 0, "Swords Dance should not calculate damage");
}

TEST_CASE(Effect_AttackUp2_CanStackMultipleTimes) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);

    // Use Swords Dance 3 times: +2, +2, +2 = +6 (capped)
    Effect_AttackUp2(ctx);
    ctx.move_failed = false;  // Reset for next use
    Effect_AttackUp2(ctx);
    ctx.move_failed = false;
    Effect_AttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == +6, "Swords Dance should stack to +6 (cap)");
}

TEST_CASE(Effect_AttackUp2_MaximumStagePlus6) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = +6;  // Already at maximum

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == +6, "Attack stage should not go above +6");
}

TEST_CASE(Effect_AttackUp2_CapsAtPlus6FromPlus5) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = +5;  // One away from cap

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == +6,
                "Swords Dance should cap at +6 (only +1 effective from +5)");
}

TEST_CASE(Effect_AttackUp2_CanRaiseFromNegativeStages) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();
    attacker.stat_stages[STAT_ATK] = -3;  // Lowered by Growl 3x

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == -1, "Swords Dance should raise from -3 to -1");
}

TEST_CASE(Effect_AttackUp2_DoesNotModifyDefender) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    int8_t original_stage = defender.stat_stages[STAT_ATK];
    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(defender.stat_stages[STAT_ATK] == original_stage,
                "Swords Dance should not affect defender's stats");
}

TEST_CASE(Effect_AttackUp2_DoesNotAffectOtherStats) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    // Only Attack should be raised
    TEST_ASSERT(attacker.stat_stages[STAT_ATK] == +2, "Attack should be +2");
    TEST_ASSERT(attacker.stat_stages[STAT_DEF] == 0, "Defense should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPEED] == 0, "Speed should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPATK] == 0, "Sp. Attack should be unchanged");
    TEST_ASSERT(attacker.stat_stages[STAT_SPDEF] == 0, "Sp. Defense should be unchanged");
}

TEST_CASE(Effect_AttackUp2_DoesNotCauseFaint) {
    auto attacker = CreateCharmander();
    auto defender = CreateBulbasaur();
    auto move = CreateSwordsDance();

    BattleContext ctx = SetupContext(&attacker, &defender, &move);
    Effect_AttackUp2(ctx);

    TEST_ASSERT(!defender.is_fainted, "Swords Dance should not cause faint (it's a status move)");
    TEST_ASSERT(defender.current_hp > 0, "Defender should still have HP");
}

// Integration test: Verify +2 Attack doubles damage output
TEST_CASE(StatStages_AttackUp2DoublesDamage) {
    auto attacker = CreateCharmander();
    auto defender1 = CreateBulbasaur();
    auto defender2 = CreateBulbasaur();
    auto tackle = CreateTackle();

    // Normal damage
    auto ctx1 = SetupContext(&attacker, &defender1, &tackle);
    Effect_Hit(ctx1);
    uint16_t normal_damage = ctx1.damage_dealt;

    // Damage with +2 Attack (2x multiplier)
    attacker.stat_stages[STAT_ATK] = +2;
    auto ctx2 = SetupContext(&attacker, &defender2, &tackle);
    Effect_Hit(ctx2);
    uint16_t boosted_damage = ctx2.damage_dealt;

    // With +2 Attack, damage should be doubled
    // Stage +2 gives multiplier (2+2)/2 = 2.0x
    TEST_ASSERT(boosted_damage > normal_damage, "Damage should be increased with +2 Attack stage");

    // Allow 2 damage rounding error
    uint16_t expected_damage = normal_damage * 2;
    int16_t diff = (int16_t)boosted_damage - (int16_t)expected_damage;
    if (diff < 0)
        diff = -diff;
    TEST_ASSERT(diff <= 2, "Boosted damage should be approximately 2x of normal damage");
}
