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
 * From pokeemerald: include/constants/battle.h:115-125
 * Uses bit flags matching Gen III exactly.
 */
namespace Status1 {
constexpr uint8_t NONE = 0;
constexpr uint8_t SLEEP = (1 << 0 | 1 << 1 | 1 << 2);  // 0x07 - First 3 bits (turns remaining)
constexpr uint8_t POISON = (1 << 3);                    // 0x08 - Regular poison
constexpr uint8_t BURN = (1 << 4);                      // 0x10 - Burn (halves Attack, 1/8 HP per turn)
constexpr uint8_t FREEZE = (1 << 5);                    // 0x20 - Freeze (can't move, 20% thaw chance)
constexpr uint8_t PARALYSIS = (1 << 6);                 // 0x40 - Paralysis (1/4 speed, 25% can't move)
constexpr uint8_t TOXIC = (1 << 7);                     // 0x80 - Badly poisoned (stacking damage)
}  // namespace Status1

}  // namespace domain
