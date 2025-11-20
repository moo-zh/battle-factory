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

    // TODO: Add stat stages, status conditions, volatile status later
};

}  // namespace state
}  // namespace battle
