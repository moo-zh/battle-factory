/**
 * @file battle/state/pokemon.hpp
 * @brief Runtime Pokemon state
 *
 * This represents a Pokemon's state during battle.
 * Includes current HP, stats, status, etc.
 */

#pragma once

#include <stdint.h>

#include "../../domain/species.hpp"

namespace battle {
namespace state {

/**
 * @brief Runtime Pokemon state during battle
 */
struct Pokemon {
    domain::Species species;
    domain::Type type1;
    domain::Type type2;

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

    // TODO: Add stat stages, volatile status (status2), abilities later
};

}  // namespace state
}  // namespace battle
