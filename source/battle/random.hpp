/**
 * @file battle/random.hpp
 * @brief Random number generation for battle mechanics
 *
 * Simple RNG for damage variance, secondary effects, etc.
 * Uses TI-84 CE's built-in random number generator.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace battle {
namespace random {

/**
 * @brief Generate a random number in range [0, max)
 * @param max Upper bound (exclusive)
 * @return Random number from 0 to max-1
 *
 * Examples:
 * - Random(100) returns 0-99 (for percentage rolls)
 * - Random(16) returns 0-15 (for 1/16 chance)
 */
inline uint16_t Random(uint16_t max) {
    if (max == 0)
        return 0;
    return rand() % max;
}

}  // namespace random
}  // namespace battle
