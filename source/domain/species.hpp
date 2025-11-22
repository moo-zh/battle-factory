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
 */
enum class Type : uint8_t {
    None = 0,
    Normal,
    Fire,
    Water,
    Electric,
    Grass,
    Poison,
    Rock,
    Ground,
    Bug,
    Flying,
    Psychic,
    Dark,
    // TODO: Add remaining types as needed
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
    // TODO: Add more species as we implement more moves
};

}  // namespace domain
