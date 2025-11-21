/**
 * @file battle/engine.cpp
 * @brief Battle Engine implementation (Phase 1: Walking Skeleton)
 */

#include "engine.hpp"

#include "context.hpp"
#include "effects/basic.hpp"

namespace battle {

// Phase 1: Hardcoded move data for Tackle
// TODO Phase 3: Replace with move database lookup
static domain::MoveData GetMoveData_Hardcoded(domain::Move move) {
    domain::MoveData data;
    data.move = move;

    // Hardcoded: only Tackle is supported in Phase 1
    if (move == domain::Move::Tackle) {
        data.type = domain::Type::Normal;
        data.power = 40;
        data.accuracy = 100;
        data.pp = 35;
        data.effect_chance = 0;
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
    // Phase 1: Hardcoded turn order - player always goes first
    // TODO Phase 4: Determine order based on speed and priority

    // Player attacks first
    if (player_action.type == ActionType::MOVE) {
        ExecuteMove(player_, enemy_, domain::Move::Tackle);  // Hardcoded: only Tackle
    }

    // Check if battle is over after player's move
    if (IsBattleOver()) {
        return;
    }

    // Enemy attacks second
    if (enemy_action.type == ActionType::MOVE) {
        ExecuteMove(enemy_, player_, domain::Move::Tackle);  // Hardcoded: only Tackle
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

    // Phase 1: Hardcoded dispatch - only Effect_Hit (Tackle)
    // TODO Phase 2: Add Effect_Paralyze
    // TODO Phase 3: Replace with function pointer table
    if (move == domain::Move::Tackle) {
        effects::Effect_Hit(ctx);
    } else {
        // Unsupported move in Phase 1 - fail silently
        ctx.move_failed = true;
    }
}

}  // namespace battle
