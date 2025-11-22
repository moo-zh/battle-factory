/**
 * @file test/host/helpers/pokemon_factory.hpp
 * @brief Pokemon creation helpers for GTest unit tests
 *
 * This file provides factory functions for creating test Pokemon with known stats.
 * These helpers reduce duplication and make tests more readable.
 */

#pragma once

#include "battle/state/pokemon.hpp"
#include "domain/species.hpp"

namespace test {
namespace helpers {

/**
 * @brief Create a Pokemon for testing with specified stats
 *
 * Creates a Pokemon with all state initialized to default values (no status,
 * neutral stat stages, no volatile conditions).
 *
 * @param species The species of the Pokemon
 * @param type1 Primary type
 * @param type2 Secondary type (use Type::None for single-type)
 * @param hp Max HP (also set as current HP)
 * @param atk Attack stat
 * @param def Defense stat
 * @param spa Special Attack stat
 * @param spd Special Defense stat
 * @param spe Speed stat
 * @return Fully initialized Pokemon ready for testing
 */
battle::state::Pokemon CreateTestPokemon(domain::Species species, domain::Type type1,
                                         domain::Type type2, uint16_t hp, uint8_t atk, uint8_t def,
                                         uint8_t spa, uint8_t spd, uint8_t spe);

/**
 * @brief Create Charmander with Gen III base stats
 *
 * Base stats: 39 HP, 52 Atk, 43 Def, 60 SpA, 50 SpD, 65 Spe
 * Type: Fire
 *
 * @return Charmander at full HP with neutral stat stages
 */
battle::state::Pokemon CreateCharmander();

/**
 * @brief Create Bulbasaur with Gen III base stats
 *
 * Base stats: 45 HP, 49 Atk, 49 Def, 65 SpA, 65 SpD, 45 Spe
 * Type: Grass/Poison
 *
 * @return Bulbasaur at full HP with neutral stat stages
 */
battle::state::Pokemon CreateBulbasaur();

/**
 * @brief Create Pikachu with Gen III base stats
 *
 * Base stats: 35 HP, 55 Atk, 30 Def, 50 SpA, 40 SpD, 90 Spe
 * Type: Electric
 *
 * @return Pikachu at full HP with neutral stat stages
 */
battle::state::Pokemon CreatePikachu();

/**
 * @brief Create Pidgey with Gen III base stats
 *
 * Base stats: 40 HP, 45 Atk, 40 Def, 35 SpA, 35 SpD, 56 Spe
 * Type: Normal/Flying
 *
 * @return Pidgey at full HP with neutral stat stages
 */
battle::state::Pokemon CreatePidgey();

/**
 * @brief Create Geodude with Gen III base stats
 *
 * Base stats: 40 HP, 80 Atk, 100 Def, 30 SpA, 30 SpD, 20 Spe
 * Type: Rock/Ground
 *
 * @return Geodude at full HP with neutral stat stages
 */
battle::state::Pokemon CreateGeodude();

/**
 * @brief Create Sandshrew with Gen III base stats
 *
 * Base stats: 50 HP, 75 Atk, 85 Def, 20 SpA, 30 SpD, 40 Spe
 * Type: Ground
 *
 * @return Sandshrew at full HP with neutral stat stages
 */
battle::state::Pokemon CreateSandshrew();

/**
 * @brief Create Skarmory with Gen III base stats
 *
 * Base stats: 65 HP, 80 Atk, 140 Def, 40 SpA, 70 SpD, 70 Spe
 * Type: Steel/Flying
 *
 * @return Skarmory at full HP with neutral stat stages
 */
battle::state::Pokemon CreateSkarmory();

/**
 * @brief Create a Pokemon with custom stats for edge case testing
 *
 * Creates a generic Pokemon (Species::None) with specified stats.
 * Useful for testing specific stat combinations without species constraints.
 *
 * @param atk Attack stat
 * @param def Defense stat
 * @param spe Speed stat
 * @param hp Max HP (default 100)
 * @return Pokemon with specified stats
 */
battle::state::Pokemon CreatePokemonWithStats(uint8_t atk, uint8_t def, uint8_t spe,
                                              uint16_t hp = 100);

}  // namespace helpers
}  // namespace test
