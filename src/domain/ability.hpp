/**
 * @file domain/ability.hpp
 * @brief Ability type definitions
 *
 * Contains Ability enum for Pokemon abilities.
 * Gen III introduced abilities - each Pokemon has exactly one ability.
 */

#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Ability enum for Pokemon abilities
 *
 * Based on Gen III ability system (pokeemerald).
 * Abilities are passive effects that trigger on specific events:
 * - Switch-in (Intimidate, Drizzle, Sand Stream)
 * - Taking damage (Rough Skin, Static)
 * - Using moves (Blaze, Overgrow for type boost)
 * - Immunity (Levitate, Water Absorb)
 */
enum class Ability : uint8_t {
    None = 0,    // No ability (for testing)
    Intimidate,  // Lowers opponent's Attack by 1 stage on switch-in

    // Future abilities:
    // Overgrow,    // Boosts Grass moves when HP < 1/3
    // Blaze,       // Boosts Fire moves when HP < 1/3
    // Torrent,     // Boosts Water moves when HP < 1/3
    // Static,      // 30% chance to paralyze on contact
    // Levitate,    // Immune to Ground-type moves
    // DrizzleGSC,  // Summons rain on switch-in (permanent in Gen III)
    // SandStream,  // Summons sandstorm on switch-in (permanent in Gen III)
    // Drought,     // Summons sun on switch-in (permanent in Gen III)
};

}  // namespace domain
