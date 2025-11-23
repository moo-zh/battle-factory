/**
 * @file battle/state/side.hpp
 * @brief Side state (per-side battle state)
 *
 * This represents per-side battle conditions that affect one player's side:
 * - Entry hazards (Stealth Rock, Spikes, Toxic Spikes)
 * - Screens (Light Screen, Reflect, Aurora Veil)
 * - Safeguard, Mist, Lucky Chant, etc.
 *
 * SideState (S domain) persists across switches and affects only one side.
 * Each player has their own SideState.
 */

#pragma once

#include <stdint.h>

namespace battle {
namespace state {

/**
 * @brief Per-side state
 *
 * State that affects one side of the battlefield.
 * Entry hazards damage Pokemon when they switch in.
 * Screens reduce damage from attacks.
 */
struct Side {
    // Entry hazards
    bool stealth_rock;  // Stealth Rock present (damages on switch-in, scaled by type effectiveness)

    // TODO: Future additions
    // - uint8_t spikes_layers;         // Spikes layers (0-3)
    // - uint8_t toxic_spikes_layers;   // Toxic Spikes layers (0-2)
    // - uint8_t reflect_turns;         // Reflect duration (halves physical damage)
    // - uint8_t light_screen_turns;    // Light Screen duration (halves special damage)
    // - uint8_t safeguard_turns;       // Safeguard duration (prevents status)
    // - uint8_t mist_turns;            // Mist duration (prevents stat drops)
};

}  // namespace state
}  // namespace battle
