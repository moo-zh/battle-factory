/**
 * @file domain/stats.hpp
 * @brief Stat type definitions and constants
 *
 * Based on pokeemerald: include/constants/pokemon.h:74-84
 */

#pragma once

#include <stdint.h>

namespace domain {

/**
 * @brief Stat indices for Pokemon stats
 *
 * These match pokeemerald's stat constants.
 * Stats HP through SPDEF are permanent stats.
 * STAT_ACC and STAT_EVASION are battle-only stats.
 */
enum Stat : uint8_t {
    STAT_HP = 0,
    STAT_ATK = 1,
    STAT_DEF = 2,
    STAT_SPEED = 3,
    STAT_SPATK = 4,
    STAT_SPDEF = 5,
    STAT_ACC = 6,      // Battle-only
    STAT_EVASION = 7,  // Battle-only
    NUM_BATTLE_STATS = 8
};

}  // namespace domain
