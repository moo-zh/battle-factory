/**
 * @file battle/context.hpp
 * @brief Battle context for move execution
 *
 * The BattleContext is passed to effect functions and commands.
 * It contains all the information needed to execute a move.
 */

#pragma once

#include <stdint.h>

#include "../domain/move.hpp"
#include "state/field.hpp"
#include "state/pokemon.hpp"
#include "state/side.hpp"

namespace battle {

/**
 * @brief Context for move execution
 *
 * This struct is created by the Engine and passed to effect functions.
 * Commands read from and write to this context to execute moves.
 */
struct BattleContext {
    // === PROVIDED BY ENGINE (read-only to effects) ===
    state::Pokemon* attacker;
    state::Pokemon* defender;
    state::Field* field;
    state::Side* attacker_side;  // Side of the attacker
    state::Side* defender_side;  // Side of the defender
    const domain::MoveData* move;

    // === EXECUTION STATE (modified by commands) ===
    bool move_failed;         // Set if move fails (miss, immunity, etc.)
    uint16_t damage_dealt;    // Actual damage calculated and applied
    uint16_t recoil_dealt;    // Recoil damage taken by attacker (for testing)
    uint16_t drain_received;  // HP drained/healed by attacker (for testing)
    bool critical_hit;        // Was this a critical hit?
    uint8_t effectiveness;    // Type effectiveness: 0=immune, 1=0.25x, 2=0.5x, 4=1x, 8=2x, 16=4x
    uint8_t hit_count;        // For multi-hit moves: number of times the move hit

    // === OVERRIDES (set by effect before CalculateDamage) ===
    uint8_t override_power;  // For variable power moves (Flail, Eruption)
    uint8_t override_type;   // For type-changing moves (Weather Ball)
};

}  // namespace battle
