/**
 * @file battle/state/field.hpp
 * @brief Field state (global battle state)
 *
 * This represents global battle conditions that affect both sides:
 * - Weather (Sandstorm, Rain, Sun, Hail)
 * - Terrain (Electric, Grassy, Misty, Psychic) [Gen VI+, not implemented]
 * - Trick Room, Magic Room, Wonder Room, Gravity [not implemented]
 *
 * FieldState (F domain) persists across switches and affects both players.
 */

#pragma once

#include <stdint.h>

#include "../../domain/weather.hpp"

namespace battle {
namespace state {

/**
 * @brief Global field state
 *
 * State that affects the entire battlefield, not tied to specific Pokemon or sides.
 */
struct Field {
    // Weather state
    domain::Weather weather;   // Current weather condition
    uint8_t weather_duration;  // Turns remaining (0 = no weather, 5 = default duration)

    // TODO: Future additions
    // - Terrain (Electric, Grassy, Misty, Psychic)
    // - Trick Room, Magic Room, Wonder Room
    // - Gravity
};

}  // namespace state
}  // namespace battle
