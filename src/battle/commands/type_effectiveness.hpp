/**
 * @file battle/commands/type_effectiveness.hpp
 * @brief Type effectiveness calculation for Gen III
 *
 * Returns effectiveness multiplier for attack type vs defender types.
 * Uses fixed-point representation: 0=immune, 2=0.5x, 4=1x, 8=2x
 *
 * Based on Gen III type chart (17 types in Gen III).
 */

#pragma once

#include <stdint.h>

#include "../../domain/species.hpp"

namespace battle {
namespace commands {

/**
 * @brief Type effectiveness chart (Gen III)
 *
 * 18x18 table indexed by [attack_type][defender_type] matching domain::Type enum exactly.
 * Values: 0=immune, 2=0.5x (not very effective), 4=1x (neutral), 8=2x (super effective)
 *
 * Type order matches domain::Type enum exactly:
 * 0=Normal, 1=Fighting, 2=Flying, 3=Poison, 4=Ground, 5=Rock, 6=Bug, 7=Ghost,
 * 8=Steel, 9=Mystery, 10=Fire, 11=Water, 12=Grass, 13=Electric, 14=Psychic,
 * 15=Ice, 16=Dragon, 17=Dark
 *
 * Mystery type (index 9) is only used by Curse and has neutral effectiveness against everything.
 *
 * Based on pokeemerald type chart.
 * Note: In Gen III, Ghost and Dark are 0.5x against Steel (changed to 1x in Gen VI+).
 */
static const uint8_t TYPE_CHART[18][18] = {
    // Defender: Nor Fig Fly Poi Gro Roc Bug Gho Ste Mys Fir Wat Gra Ele Psy Ice Dra Dar
    /* Normal   */ {4, 4, 4, 4, 4, 2, 4, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4},
    /* Fighting */ {8, 4, 2, 2, 4, 8, 2, 0, 8, 4, 4, 4, 4, 4, 2, 8, 4, 8},
    /* Flying   */ {4, 8, 4, 4, 4, 2, 8, 4, 2, 4, 4, 4, 8, 2, 4, 4, 4, 4},
    /* Poison   */ {4, 4, 4, 2, 2, 2, 4, 2, 0, 4, 4, 4, 8, 4, 4, 4, 4, 4},
    /* Ground   */ {4, 4, 0, 8, 4, 8, 2, 4, 8, 4, 8, 4, 2, 8, 4, 4, 4, 4},
    /* Rock     */ {4, 2, 8, 4, 2, 4, 8, 4, 2, 4, 8, 4, 4, 4, 4, 8, 4, 4},
    /* Bug      */ {4, 2, 2, 2, 4, 4, 4, 2, 2, 4, 2, 4, 8, 4, 8, 4, 4, 8},
    /* Ghost    */ {0, 4, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 4, 8, 4, 4, 2},
    /* Steel    */ {4, 4, 4, 4, 4, 8, 4, 4, 2, 4, 2, 2, 4, 2, 4, 8, 4, 4},
    /* Mystery  */ {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
    /* Fire     */ {4, 4, 4, 4, 4, 2, 8, 4, 8, 4, 2, 2, 8, 4, 4, 8, 2, 4},
    /* Water    */ {4, 4, 4, 4, 8, 8, 4, 4, 4, 4, 8, 2, 2, 4, 4, 4, 2, 4},
    /* Grass    */ {4, 4, 2, 2, 8, 8, 2, 4, 2, 4, 2, 8, 2, 4, 4, 4, 2, 4},
    /* Electric */ {4, 4, 8, 4, 0, 4, 4, 4, 4, 4, 4, 8, 2, 2, 4, 4, 2, 4},
    /* Psychic  */ {4, 8, 4, 8, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 2, 4, 4, 0},
    /* Ice      */ {4, 4, 8, 4, 8, 4, 4, 4, 2, 4, 2, 2, 8, 4, 4, 2, 8, 4},
    /* Dragon   */ {4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 8, 4},
    /* Dark     */ {4, 2, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 4, 8, 4, 4, 2},
};

/**
 * @brief Get type effectiveness multiplier
 *
 * @param attack_type The type of the attacking move
 * @param defender_type The type of the defender
 * @return Effectiveness multiplier (0=immune, 2=0.5x, 4=1x, 8=2x)
 */
inline uint8_t GetSingleTypeEffectiveness(domain::Type attack_type, domain::Type defender_type) {
    // Bounds check (should never happen in practice)
    if (static_cast<uint8_t>(attack_type) >= 18 || static_cast<uint8_t>(defender_type) >= 18) {
        return 4;  // Neutral if out of bounds
    }

    return TYPE_CHART[static_cast<uint8_t>(attack_type)][static_cast<uint8_t>(defender_type)];
}

/**
 * @brief Get combined type effectiveness for dual-type defender
 *
 * @param attack_type The type of the attacking move
 * @param defender_type1 Primary type of defender
 * @param defender_type2 Secondary type of defender (same as type1 if monotype)
 * @return Combined effectiveness (0=immune, 1=0.25x, 2=0.5x, 4=1x, 8=2x, 16=4x)
 *
 * Multiplies effectiveness against both types:
 * - 2x * 2x = 4x (super effective against both types)
 * - 2x * 0.5x = 1x (cancel out)
 * - 0.5x * 0.5x = 0.25x (resists both types)
 * - Anything * 0x = 0x (immune)
 */
inline uint8_t GetTypeEffectiveness(domain::Type attack_type, domain::Type defender_type1,
                                    domain::Type defender_type2) {
    uint8_t eff1 = GetSingleTypeEffectiveness(attack_type, defender_type1);
    uint8_t eff2 = GetSingleTypeEffectiveness(attack_type, defender_type2);

    // Multiply effectiveness values (both use same scale: 4 = 1x)
    // Result: 16 = 4x, 8 = 2x, 4 = 1x, 2 = 0.5x, 1 = 0.25x, 0 = immune
    uint16_t combined = (eff1 * eff2) / 4;  // Divide by 4 to normalize (4 * 4 / 4 = 4 = 1x)

    // Clamp to uint8_t range
    if (combined > 255)
        combined = 255;
    return static_cast<uint8_t>(combined);
}

}  // namespace commands
}  // namespace battle
