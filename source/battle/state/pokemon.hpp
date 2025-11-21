/**
 * @file battle/state/pokemon.hpp
 * @brief Runtime Pokemon state
 *
 * This represents a Pokemon's state during battle.
 * Includes current HP, stats, status, etc.
 */

#pragma once

#include <stdint.h>

#include "../../domain/move.hpp"
#include "../../domain/species.hpp"
#include "../../domain/stats.hpp"

namespace battle {
namespace state {

/**
 * @brief Runtime Pokemon state during battle
 */
struct Pokemon {
    domain::Species species;
    domain::Type type1;
    domain::Type type2;
    uint8_t level;

    // Base stats (not modified by stages)
    uint8_t attack;
    uint8_t defense;
    uint8_t sp_attack;
    uint8_t sp_defense;
    uint8_t speed;

    // Runtime state
    uint16_t max_hp;
    uint16_t current_hp;
    bool is_fainted;

    // Status conditions
    uint8_t status1;  // Primary status: Sleep, Poison, Burn, Freeze, Paralysis

    // Stat stages (-6 to +6, with 0 being neutral)
    // Stages apply multipliers to stats during damage calculation
    // Order: ATK, DEF, SPEED, SPATK, SPDEF, ACC, EVASION
    int8_t stat_stages[domain::NUM_BATTLE_STATS];

    // Protection state
    bool is_protected;      // Volatile flag: protected this turn (cleared each turn)
    uint8_t protect_count;  // Consecutive successful Protect uses (for success rate calc)

    // Two-turn move state
    bool is_charging;            // Volatile flag: currently charging a two-turn move
    domain::Move charging_move;  // Which move is being charged (for move validation)

    // TODO: Add volatile status (status2), abilities later
};

}  // namespace state
}  // namespace battle
