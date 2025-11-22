/**
 * @file battle/random.hpp
 * @brief Random number generation for battle mechanics
 *
 * PCG32 random number generator for battle calculations.
 * High-quality PRNG suitable for game mechanics (accuracy, crits, damage variance).
 *
 * Reference: https://www.pcg-random.org/
 * Algorithm: PCG XSH RR 64/32 (LCG)
 */

#pragma once

#include <stdint.h>

namespace battle {
namespace random {

/**
 * @brief Initialize RNG with seed
 * @param seed Random seed (0 = use rtc_Time() for hardware entropy)
 *
 * For deterministic testing: Initialize(0x12345678)
 * For normal gameplay: Initialize() uses RTC automatically
 */
void Initialize(uint32_t seed = 0);

/**
 * @brief Generate a random number in range [0, max)
 * @param max Upper bound (exclusive)
 * @return Random number from 0 to max-1
 *
 * Examples:
 * - Random(100) returns 0-99 (for percentage rolls)
 * - Random(16) returns 0-15 (for 1/16 chance)
 */
uint16_t Random(uint16_t max);

}  // namespace random
}  // namespace battle
