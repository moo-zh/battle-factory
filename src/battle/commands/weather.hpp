/**
 * @file battle/commands/weather.hpp
 * @brief Weather-setting commands
 *
 * Commands for setting and manipulating weather conditions.
 */

#pragma once

#include "../../domain/weather.hpp"
#include "../context.hpp"

namespace battle {
namespace commands {

/**
 * @brief Set weather on the field
 * @param ctx Battle context (provides field state)
 * @param weather Weather type to set
 * @param duration Duration in turns (default 5 for normal weather moves)
 *
 * Sets the weather condition. If weather is already active, it is replaced.
 * Weather duration of 0 clears weather.
 *
 * Based on pokeemerald's SetWeather function.
 */
inline void SetWeather(BattleContext& ctx, domain::Weather weather, uint8_t duration = 5) {
    // Guard: check move_failed (standard command pattern)
    if (ctx.move_failed) {
        return;
    }

    // Set weather state
    ctx.field->weather = weather;
    ctx.field->weather_duration = duration;

    // TODO: Display weather message
    // - "A sandstorm kicked up!"
    // - "It started to rain!"
    // - "The sunlight turned harsh!"
    // - "It started to hail!"
}

}  // namespace commands
}  // namespace battle
