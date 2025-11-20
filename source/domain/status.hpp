/**
 * @file domain/status.hpp
 * @brief Status condition constants
 *
 * Defines bit flags for primary status conditions (status1).
 * Based on Gen III pokeemerald status flags.
 */

#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Primary status condition flags (status1)
 *
 * These are mutually exclusive - a Pokemon can only have one at a time.
 * The value 0 means no status.
 *
 * From pokeemerald: include/constants/battle.h
 */
namespace Status1 {
constexpr uint8_t NONE = 0;
constexpr uint8_t SLEEP = 1;       // Sleep (1-3 turns in Gen III)
constexpr uint8_t POISON = 2;      // Regular poison
constexpr uint8_t BURN = 4;        // Burn (halves Attack, 1/8 HP per turn)
constexpr uint8_t FREEZE = 8;      // Freeze (can't move, 20% thaw chance)
constexpr uint8_t PARALYSIS = 16;  // Paralysis (1/4 speed, 25% can't move)
constexpr uint8_t TOXIC = 32;      // Badly poisoned (stacking damage)
}  // namespace Status1

}  // namespace domain
