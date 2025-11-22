/**
 * @file test/host/helpers/battle_helpers.cpp
 * @brief Implementation of battle context and move data factories
 */

#include "battle_helpers.hpp"

namespace test {
namespace helpers {

battle::BattleContext CreateBattleContext(battle::state::Pokemon* attacker,
                                          battle::state::Pokemon* defender) {
    battle::BattleContext ctx;
    ctx.attacker = attacker;
    ctx.defender = defender;
    ctx.move = nullptr;
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // Default to 1x (neutral, using 4 = 1.0 in fixed-point)
    ctx.override_power = 0;
    ctx.override_type = 0;
    ctx.recoil_dealt = 0;
    ctx.drain_received = 0;
    ctx.hit_count = 0;
    return ctx;
}

battle::BattleContext CreateBattleContext(battle::state::Pokemon* attacker,
                                          battle::state::Pokemon* defender,
                                          const domain::MoveData* move) {
    battle::BattleContext ctx = CreateBattleContext(attacker, defender);
    ctx.move = move;
    return ctx;
}

// ============================================================================
// MOVE DATA FACTORIES
// ============================================================================

domain::MoveData CreateTackle() {
    domain::MoveData tackle;
    tackle.move = domain::Move::Tackle;
    tackle.type = domain::Type::Normal;
    tackle.power = 35;
    tackle.accuracy = 95;
    tackle.pp = 35;
    tackle.effect_chance = 0;
    tackle.priority = 0;
    return tackle;
}

domain::MoveData CreateEmber() {
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

domain::MoveData CreateThunderWave() {
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

domain::MoveData CreateGrowl() {
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

domain::MoveData CreateTailWhip() {
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

domain::MoveData CreateSwordsDance() {
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

domain::MoveData CreateIronDefense() {
    domain::MoveData id;
    id.move = domain::Move::IronDefense;
    id.type = domain::Type::Steel;
    id.power = 0;
    id.accuracy = 0;  // Self-targeting
    id.pp = 15;
    id.effect_chance = 0;
    id.priority = 0;
    return id;
}

domain::MoveData CreateAgility() {
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

domain::MoveData CreateTailGlow() {
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

domain::MoveData CreateFakeTears() {
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

domain::MoveData CreateAmnesia() {
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

domain::MoveData CreateStringShot() {
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

domain::MoveData CreateDoubleEdge() {
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

domain::MoveData CreateGigaDrain() {
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

domain::MoveData CreateFuryAttack() {
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

domain::MoveData CreateProtect() {
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

domain::MoveData CreateSolarBeam() {
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

domain::MoveData CreateFly() {
    domain::MoveData fly;
    fly.move = domain::Move::Fly;
    fly.type = domain::Type::Flying;
    fly.power = 70;
    fly.accuracy = 95;
    fly.pp = 15;
    fly.effect_chance = 0;
    fly.priority = 0;
    return fly;
}

domain::MoveData CreateSubstitute() {
    domain::MoveData sub;
    sub.move = domain::Move::Substitute;
    sub.type = domain::Type::Normal;
    sub.power = 0;
    sub.accuracy = 0;  // Self-targeting, never misses
    sub.pp = 10;
    sub.effect_chance = 0;
    sub.priority = 0;
    return sub;
}

domain::MoveData CreateBatonPass() {
    domain::MoveData bp;
    bp.move = domain::Move::BatonPass;
    bp.type = domain::Type::Normal;
    bp.power = 0;
    bp.accuracy = 0;  // Self-targeting, never misses
    bp.pp = 40;
    bp.effect_chance = 0;
    bp.priority = 0;
    return bp;
}

}  // namespace helpers
}  // namespace test
