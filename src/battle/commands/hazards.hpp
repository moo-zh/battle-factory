/**
 * @file battle/commands/hazards.hpp
 * @brief Entry hazard damage application
 *
 * Handles damage from entry hazards when a Pokemon switches in:
 * - Stealth Rock (type-based damage)
 * - Spikes (layered damage, not implemented)
 * - Toxic Spikes (poison application, not implemented)
 */

#pragma once

#include <stdint.h>

#include "../../domain/species.hpp"
#include "../state/pokemon.hpp"
#include "../state/side.hpp"
#include "type_effectiveness.hpp"

namespace battle {
namespace commands {

/**
 * @brief Apply Stealth Rock damage to a Pokemon switching in
 *
 * @param pokemon The Pokemon switching in
 * @param side The side of the battlefield (contains hazard state)
 *
 * Damage formula: (max HP / 8) * type effectiveness vs Rock
 * - 4x weak to Rock (Fire/Flying): 50% max HP
 * - 2x weak to Rock (Fire, Ice, Flying, Bug): 25% max HP
 * - Neutral (most types): 12.5% max HP
 * - 0.5x resist (Fighting, Ground, Steel): 6.25% max HP
 * - 0.25x double resist (Fighting/Steel, Ground/Steel): 3.125% max HP
 *
 * Based on pokeemerald's VARIOUS_TRY_ACTIVATE_STEALTH_ROCKS
 */
inline void ApplyStealthRockDamage(state::Pokemon& pokemon, const state::Side& side) {
    if (!side.stealth_rock) {
        return;  // No stealth rocks on this side
    }

    if (pokemon.is_fainted) {
        return;  // Already fainted, no damage
    }

    // Calculate type effectiveness of Rock-type attack vs this Pokemon
    uint8_t effectiveness = GetTypeEffectiveness(domain::Type::Rock, pokemon.type1, pokemon.type2);

    // Apply type effectiveness to base damage (1/8 max HP)
    // effectiveness: 0=immune, 1=0.25x, 2=0.5x, 4=1x, 8=2x, 16=4x
    // Formula: (max_hp * effectiveness) / 32
    // This avoids precision loss from dividing max_hp by 8 first
    uint32_t damage = (pokemon.max_hp * effectiveness) / 32;

    // Minimum 1 damage if not immune (unless max HP < 32, then 0)
    if (effectiveness > 0 && damage == 0 && pokemon.max_hp >= 32) {
        damage = 1;
    }

    // Apply damage (clamped at 0 HP)
    if (damage >= pokemon.current_hp) {
        pokemon.current_hp = 0;
        pokemon.is_fainted = true;
    } else {
        pokemon.current_hp -= static_cast<uint16_t>(damage);
    }

    // TODO: Display message: "[Pokemon] was hurt by the pointed stones!"
}

/**
 * @brief Apply all entry hazard effects when a Pokemon switches in
 *
 * @param pokemon The Pokemon switching in
 * @param side The side of the battlefield
 *
 * Applies entry hazard damage in order:
 * 1. Stealth Rock (type-based)
 * 2. Spikes (layer-based, not implemented)
 * 3. Toxic Spikes (poison, not implemented)
 *
 * Based on pokeemerald's switch-in hazard application order
 */
inline void ApplySwitchInHazards(state::Pokemon& pokemon, const state::Side& side) {
    // Apply Stealth Rock damage
    ApplyStealthRockDamage(pokemon, side);

    // TODO: Apply Spikes damage (if layers > 0)
    // TODO: Apply Toxic Spikes poison (if layers > 0)
}

}  // namespace commands
}  // namespace battle
