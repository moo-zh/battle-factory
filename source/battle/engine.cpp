/**
 * @file battle/engine.cpp
 * @brief Battle Engine implementation (Phase 2: Multi-effect support)
 */

#include "engine.hpp"

#include "context.hpp"
#include "effects/basic.hpp"

namespace battle {

// Phase 2: Hardcoded move data for Tackle and Thunder Wave
// TODO Phase 3: Replace with move database lookup
static domain::MoveData GetMoveData_Hardcoded(domain::Move move) {
    domain::MoveData data;
    data.move = move;

    // Phase 2: Support Tackle (damage) and Thunder Wave (status-only)
    if (move == domain::Move::Tackle) {
        data.type = domain::Type::Normal;
        data.power = 40;
        data.accuracy = 100;
        data.pp = 35;
        data.effect_chance = 0;
    } else if (move == domain::Move::ThunderWave) {
        data.type = domain::Type::Electric;
        data.power = 0;  // Status-only move
        data.accuracy = 100;
        data.pp = 20;
        data.effect_chance = 100;  // Always paralyzes if it hits
    } else {
        // Unsupported move - return dummy data that will fail
        data.type = domain::Type::Normal;
        data.power = 0;
        data.accuracy = 0;
        data.pp = 0;
        data.effect_chance = 0;
    }

    return data;
}

void BattleEngine::InitBattle(const state::Pokemon& player_pokemon,
                              const state::Pokemon& enemy_pokemon) {
    player_ = player_pokemon;
    enemy_ = enemy_pokemon;
}

void BattleEngine::ExecuteTurn(const BattleAction& player_action,
                               const BattleAction& enemy_action) {
    // Phase 2: Still hardcoded turn order - player always goes first
    // TODO Phase 4: Determine order based on speed and priority

    // Player attacks first
    if (player_action.type == ActionType::MOVE) {
        ExecuteMove(player_, enemy_, player_action.move);  // Phase 2: Use move from action
    }

    // Check if battle is over after player's move
    if (IsBattleOver()) {
        return;
    }

    // Enemy attacks second
    if (enemy_action.type == ActionType::MOVE) {
        ExecuteMove(enemy_, player_, enemy_action.move);  // Phase 2: Use move from action
    }
}

bool BattleEngine::IsBattleOver() const {
    return player_.is_fainted || enemy_.is_fainted;
}

void BattleEngine::ExecuteMove(state::Pokemon& attacker, state::Pokemon& defender,
                               domain::Move move) {
    // Set up battle context
    BattleContext ctx;
    ctx.attacker = &attacker;
    ctx.defender = &defender;

    // Get move data (Phase 1: hardcoded lookup)
    domain::MoveData move_data = GetMoveData_Hardcoded(move);
    ctx.move = &move_data;

    // Initialize execution state
    ctx.move_failed = false;
    ctx.damage_dealt = 0;
    ctx.recoil_dealt = 0;
    ctx.drain_received = 0;
    ctx.critical_hit = false;
    ctx.effectiveness = 4;  // 1.0x (normal effectiveness)
    ctx.hit_count = 0;
    ctx.override_power = 0;
    ctx.override_type = 0;

    // Phase 2: Hardcoded dispatch - Effect_Hit and Effect_Paralyze
    // TODO Phase 3: Replace with function pointer table
    if (move == domain::Move::Tackle) {
        effects::Effect_Hit(ctx);
    } else if (move == domain::Move::ThunderWave) {
        effects::Effect_Paralyze(ctx);
    } else {
        // Unsupported move in Phase 2 - fail silently
        ctx.move_failed = true;
    }
}

}  // namespace battle
