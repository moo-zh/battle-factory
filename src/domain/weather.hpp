/**
 * @file domain/weather.hpp
 * @brief Weather type definitions
 *
 * Contains Weather enum for battlefield weather conditions.
 */

#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Weather conditions
 *
 * Based on Gen III weather mechanics.
 * Weather affects various battle mechanics:
 * - Sandstorm: Damages non-Rock/Ground/Steel types (1/16 max HP)
 * - Rain: Boosts Water moves, weakens Fire moves
 * - Sun: Boosts Fire moves, weakens Water moves
 * - Hail: Damages non-Ice types (1/16 max HP)
 */
enum class Weather : uint8_t {
    None = 0,   // Clear weather
    Sandstorm,  // Sandstorm (damages non-Rock/Ground/Steel)
    Rain,       // Rain (boosts Water, weakens Fire)
    Sun,        // Harsh sunlight (boosts Fire, weakens Water)
    Hail,       // Hail (damages non-Ice)
};

}  // namespace domain
