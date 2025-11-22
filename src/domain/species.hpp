/**
 * @file domain/species.hpp
 * @brief Species type definitions
 *
 * Contains Species enum and SpeciesData struct.
 * For now, enum and data are in the same file (will split when we have 15+ species)
 */

#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Type enum for Pokemon types
 *
 * Based on Gen III type system (pokeemerald)
 */
enum class Type : uint8_t {
    Normal = 0,
    Fighting,
    Flying,
    Poison,
    Ground,
    Rock,
    Bug,
    Ghost,
    Steel,
    Mystery,  // Gen II "???" type, rarely used
    Fire,
    Water,
    Grass,
    Electric,
    Psychic,
    Ice,
    Dragon,
    Dark,
    None = 255,  // No type / type slot not used
};

/**
 * @brief Species enum for Pokemon species
 */
enum class Species : uint8_t {
    None = 0,
    Charmander,
    Bulbasaur,
    Pikachu,
    Pidgey,
    Geodude,
    Sandshrew,
    Skarmory,
    // TODO: Add more species as we implement more moves
};

}  // namespace domain
