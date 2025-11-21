/**
 * @file domain/move.hpp
 * @brief Move type definitions
 *
 * Contains Move enum and MoveData struct.
 */

#pragma once

#include <stdint.h>

#include "species.hpp"  // For Type enum

namespace domain {

/**
 * @brief Move enum for Pokemon moves
 */
enum class Move : uint8_t {
    None = 0,
    Tackle,
    Ember,
    ThunderWave,
    Growl,
    TailWhip,
    SwordsDance,
    DoubleEdge,
    GigaDrain,
    IronDefense,
    StringShot,
    Agility,
    // TODO: Add more moves as we implement them
};

/**
 * @brief Move data structure
 */
struct MoveData {
    Move move;
    Type type;
    uint8_t power;
    uint8_t accuracy;
    uint8_t pp;
    uint8_t effect_chance;  // Secondary effect chance (e.g., 10 for 10% burn)
};

}  // namespace domain
