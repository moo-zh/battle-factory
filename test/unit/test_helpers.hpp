/**
 * @file test/unit/test_helpers.hpp
 * @brief Common test helper functions for unit tests
 *
 * This file contains shared factories for creating Pokemon, moves, and battle contexts
 * to reduce duplication across test files.
 */

#pragma once

#include "../../source/battle/context.hpp"
#include "../../source/battle/effects/basic.hpp"
#include "../../source/battle/state/pokemon.hpp"
#include "../../source/domain/move.hpp"
#include "../../source/domain/species.hpp"
#include "../../source/domain/stats.hpp"

// Bring domain enums and battle types into scope for convenience in tests
using namespace domain;
using namespace battle;
using namespace battle::effects;

// ============================================================================
// POKEMON FACTORIES
// ============================================================================

/**
 * @brief Create a Pokemon for testing with specified stats
 */
inline battle::state::Pokemon CreateTestPokemon(domain::Species species, domain::Type type1,
                                                domain::Type type2, uint16_t hp, uint8_t atk,
                                                uint8_t def, uint8_t spa, uint8_t spd,
                                                uint8_t spe) {
    battle::state::Pokemon p;
    p.species = species;
    p.type1 = type1;
    p.type2 = type2;
    p.level = 5;
    p.attack = atk;
    p.defense = def;
    p.sp_attack = spa;
    p.sp_defense = spd;
    p.speed = spe;
    p.max_hp = hp;
    p.current_hp = hp;
    p.is_fainted = false;
    p.status1 = 0;  // No status

    // Initialize stat stages to 0 (neutral)
    for (int i = 0; i < 8; i++) {
        p.stat_stages[i] = 0;
    }

    // Initialize protection state
    p.is_protected = false;
    p.protect_count = 0;

    // Initialize two-turn move state
    p.is_charging = false;
    p.charging_move = domain::Move::None;

    return p;
}

/**
 * @brief Create Charmander with Gen III base stats
 * Base stats: 39 HP, 52 Atk, 43 Def, 60 SpA, 50 SpD, 65 Spe
 */
inline battle::state::Pokemon CreateCharmander() {
    return CreateTestPokemon(domain::Species::Charmander, domain::Type::Fire, domain::Type::None,
                             39,   // HP (using base stat as max HP for simplicity)
                             52,   // Attack
                             43,   // Defense
                             60,   // Sp. Attack
                             50,   // Sp. Defense
                             65);  // Speed
}

/**
 * @brief Create Bulbasaur with Gen III base stats
 * Base stats: 45 HP, 49 Atk, 49 Def, 65 SpA, 65 SpD, 45 Spe
 */
inline battle::state::Pokemon CreateBulbasaur() {
    return CreateTestPokemon(domain::Species::Bulbasaur, domain::Type::Grass, domain::Type::Poison,
                             45,   // HP
                             49,   // Attack
                             49,   // Defense
                             65,   // Sp. Attack
                             65,   // Sp. Defense
                             45);  // Speed
}

/**
 * @brief Create Pikachu with Gen III base stats
 * Base stats: 35 HP, 55 Atk, 30 Def, 50 SpA, 40 SpD, 90 Spe
 */
inline battle::state::Pokemon CreatePikachu() {
    return CreateTestPokemon(domain::Species::Pikachu, domain::Type::Electric, domain::Type::None,
                             35,   // HP
                             55,   // Attack
                             30,   // Defense
                             50,   // Sp. Attack
                             40,   // Sp. Defense
                             90);  // Speed
}

// ============================================================================
// MOVE FACTORIES
// ============================================================================

/**
 * @brief Create the Tackle move data
 * Gen III: 35 power, 95 accuracy, Normal type
 */
inline domain::MoveData CreateTackle() {
    domain::MoveData tackle;
    tackle.move = domain::Move::Tackle;
    tackle.type = domain::Type::Normal;
    tackle.power = 35;
    tackle.accuracy = 95;
    tackle.pp = 35;
    tackle.effect_chance = 0;  // No secondary effect
    tackle.priority = 0;
    return tackle;
}

/**
 * @brief Create the Ember move data
 * Gen III: 40 power, 100 accuracy, Fire type, 10% burn chance
 */
inline domain::MoveData CreateEmber() {
    domain::MoveData ember;
    ember.move = domain::Move::Ember;
    ember.type = domain::Type::Fire;
    ember.power = 40;
    ember.accuracy = 100;
    ember.pp = 25;
    ember.effect_chance = 10;  // 10% burn chance
    ember.priority = 0;
    return ember;
}

/**
 * @brief Create the Thunder Wave move data
 * Gen III: 0 power, 100 accuracy, Electric type
 */
inline domain::MoveData CreateThunderWave() {
    domain::MoveData tw;
    tw.move = domain::Move::ThunderWave;
    tw.type = domain::Type::Electric;
    tw.power = 0;
    tw.accuracy = 100;
    tw.pp = 20;
    tw.effect_chance = 0;
    tw.priority = 0;
    return tw;
}

/**
 * @brief Create the Growl move data
 * Gen III: 0 power, 100 accuracy, Normal type
 */
inline domain::MoveData CreateGrowl() {
    domain::MoveData growl;
    growl.move = domain::Move::Growl;
    growl.type = domain::Type::Normal;
    growl.power = 0;
    growl.accuracy = 100;
    growl.pp = 40;
    growl.effect_chance = 0;
    growl.priority = 0;
    return growl;
}

/**
 * @brief Create the Tail Whip move data
 * Gen III: 0 power, 100 accuracy, Normal type
 */
inline domain::MoveData CreateTailWhip() {
    domain::MoveData tw;
    tw.move = domain::Move::TailWhip;
    tw.type = domain::Type::Normal;
    tw.power = 0;
    tw.accuracy = 100;
    tw.pp = 30;
    tw.effect_chance = 0;
    tw.priority = 0;
    return tw;
}

/**
 * @brief Create the Swords Dance move data
 * Gen III: 0 power, 0 accuracy (self-targeting), Normal type
 */
inline domain::MoveData CreateSwordsDance() {
    domain::MoveData sd;
    sd.move = domain::Move::SwordsDance;
    sd.type = domain::Type::Normal;
    sd.power = 0;
    sd.accuracy = 0;  // Self-targeting
    sd.pp = 30;
    sd.effect_chance = 0;
    sd.priority = 0;
    return sd;
}

/**
 * @brief Create the Double-Edge move data
 * Gen III: 120 power, 100 accuracy, Normal type, 33% recoil
 */
inline domain::MoveData CreateDoubleEdge() {
    domain::MoveData de;
    de.move = domain::Move::DoubleEdge;
    de.type = domain::Type::Normal;
    de.power = 120;
    de.accuracy = 100;
    de.pp = 15;
    de.effect_chance = 0;
    de.priority = 0;
    return de;
}

/**
 * @brief Create the Giga Drain move data
 * Gen III: 60 power, 100 accuracy, Grass type, 50% drain
 */
inline domain::MoveData CreateGigaDrain() {
    domain::MoveData gd;
    gd.move = domain::Move::GigaDrain;
    gd.type = domain::Type::Grass;
    gd.power = 60;
    gd.accuracy = 100;
    gd.pp = 5;
    gd.effect_chance = 0;
    gd.priority = 0;
    return gd;
}

/**
 * @brief Create the Iron Defense move data
 * Gen III: 0 power, 0 accuracy (self-targeting), Steel type
 */
inline domain::MoveData CreateIronDefense() {
    domain::MoveData id;
    id.move = domain::Move::IronDefense;
    id.type = domain::Type::Normal;
    id.power = 0;
    id.accuracy = 0;  // Self-targeting
    id.pp = 15;
    id.effect_chance = 0;
    id.priority = 0;
    return id;
}

/**
 * @brief Create the String Shot move data
 * Gen III: 0 power, 95 accuracy, Bug type
 */
inline domain::MoveData CreateStringShot() {
    domain::MoveData ss;
    ss.move = domain::Move::StringShot;
    ss.type = domain::Type::Bug;
    ss.power = 0;
    ss.accuracy = 95;
    ss.pp = 40;
    ss.effect_chance = 0;
    ss.priority = 0;
    return ss;
}

/**
 * @brief Create the Agility move data
 * Gen III: 0 power, 0 accuracy (self-targeting), Psychic type
 */
inline domain::MoveData CreateAgility() {
    domain::MoveData ag;
    ag.move = domain::Move::Agility;
    ag.type = domain::Type::Psychic;
    ag.power = 0;
    ag.accuracy = 0;  // Self-targeting
    ag.pp = 30;
    ag.effect_chance = 0;
    ag.priority = 0;
    return ag;
}

/**
 * @brief Create the Tail Glow move data
 * Gen III: 0 power, 0 accuracy (self-targeting), Bug type
 */
inline domain::MoveData CreateTailGlow() {
    domain::MoveData tg;
    tg.move = domain::Move::TailGlow;
    tg.type = domain::Type::Bug;
    tg.power = 0;
    tg.accuracy = 0;  // Self-targeting
    tg.pp = 20;
    tg.effect_chance = 0;
    tg.priority = 0;
    return tg;
}

/**
 * @brief Create the Fake Tears move data
 * Gen III: 0 power, 100 accuracy, Dark type
 */
inline domain::MoveData CreateFakeTears() {
    domain::MoveData ft;
    ft.move = domain::Move::FakeTears;
    ft.type = domain::Type::Dark;
    ft.power = 0;
    ft.accuracy = 100;
    ft.pp = 20;
    ft.effect_chance = 0;
    ft.priority = 0;
    return ft;
}

/**
 * @brief Create the Amnesia move data
 * Gen III: 0 power, 0 accuracy (self-targeting), Psychic type
 */
inline domain::MoveData CreateAmnesia() {
    domain::MoveData am;
    am.move = domain::Move::Amnesia;
    am.type = domain::Type::Psychic;
    am.power = 0;
    am.accuracy = 0;  // Self-targeting
    am.pp = 20;
    am.effect_chance = 0;
    am.priority = 0;
    return am;
}

/**
 * @brief Create the Fury Attack move data
 * Gen III: 15 power, 85 accuracy, Normal type, hits 2-5 times
 */
inline domain::MoveData CreateFuryAttack() {
    domain::MoveData fa;
    fa.move = domain::Move::FuryAttack;
    fa.type = domain::Type::Normal;
    fa.power = 15;
    fa.accuracy = 85;
    fa.pp = 20;
    fa.effect_chance = 0;
    fa.priority = 0;
    return fa;
}

/**
 * @brief Create the Protect move data
 * Gen III: 0 power, 0 accuracy (always hits), Normal type, +4 priority
 */
inline domain::MoveData CreateProtect() {
    domain::MoveData protect;
    protect.move = domain::Move::Protect;
    protect.type = domain::Type::Normal;
    protect.power = 0;
    protect.accuracy = 0;  // Self-targeting, cannot miss
    protect.pp = 10;
    protect.effect_chance = 0;
    protect.priority = 4;  // +4 priority
    return protect;
}

/**
 * @brief Create the Solar Beam move data
 * Gen III: 120 power, 100 accuracy, Grass type, two-turn move
 */
inline domain::MoveData CreateSolarBeam() {
    domain::MoveData sb;
    sb.move = domain::Move::SolarBeam;
    sb.type = domain::Type::Grass;
    sb.power = 120;
    sb.accuracy = 100;
    sb.pp = 10;
    sb.effect_chance = 0;
    sb.priority = 0;
    return sb;
}

// ============================================================================
// BATTLE CONTEXT SETUP
// ============================================================================

/**
 * @brief Setup a battle context for testing
 */
inline battle::BattleContext SetupContext(battle::state::Pokemon* attacker,
                                          battle::state::Pokemon* defender,
                                          const domain::MoveData* move) {
    battle::BattleContext ctx;
    ctx.attacker = attacker;
    ctx.defender = defender;
    ctx.move = move;
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // Default to 1x (neutral)
    ctx.override_power = 0;
    ctx.override_type = 0;
    ctx.recoil_dealt = 0;
    ctx.drain_received = 0;
    ctx.hit_count = 0;
    return ctx;
}
