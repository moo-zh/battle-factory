/**
 * @file test/host/helpers/battle_helpers.hpp
 * @brief Battle context and move data creation helpers for GTest unit tests
 *
 * This file provides factory functions for creating battle contexts and move data
 * to reduce boilerplate in tests.
 */

#pragma once

#include "battle/context.hpp"
#include "battle/state/pokemon.hpp"
#include "domain/move.hpp"

namespace test {
namespace helpers {

/**
 * @brief Create a battle context for testing
 *
 * Creates a BattleContext with all fields initialized to default/safe values.
 * Pointers to attacker and defender are set, but move must be assigned separately.
 *
 * @param attacker Pointer to attacking Pokemon
 * @param defender Pointer to defending Pokemon
 * @return Initialized BattleContext ready for testing
 */
battle::BattleContext CreateBattleContext(battle::state::Pokemon* attacker,
                                          battle::state::Pokemon* defender);

/**
 * @brief Create a battle context with move data
 *
 * Creates a BattleContext with pointers to attacker, defender, and move data.
 *
 * @param attacker Pointer to attacking Pokemon
 * @param defender Pointer to defending Pokemon
 * @param move Pointer to move data
 * @return Initialized BattleContext ready for testing
 */
battle::BattleContext CreateBattleContext(battle::state::Pokemon* attacker,
                                          battle::state::Pokemon* defender,
                                          const domain::MoveData* move);

// ============================================================================
// MOVE DATA FACTORIES
// ============================================================================

/**
 * @brief Create the Tackle move data
 * Gen III: 35 power, 95 accuracy, Normal type
 */
domain::MoveData CreateTackle();

/**
 * @brief Create the Ember move data
 * Gen III: 40 power, 100 accuracy, Fire type, 10% burn chance
 */
domain::MoveData CreateEmber();

/**
 * @brief Create the Thunder Wave move data
 * Gen III: 0 power, 100 accuracy, Electric type
 */
domain::MoveData CreateThunderWave();

/**
 * @brief Create the Growl move data
 * Gen III: 0 power, 100 accuracy, Normal type
 */
domain::MoveData CreateGrowl();

/**
 * @brief Create the Tail Whip move data
 * Gen III: 0 power, 100 accuracy, Normal type
 */
domain::MoveData CreateTailWhip();

/**
 * @brief Create the Swords Dance move data
 * Gen III: 0 power, self-targeting, Normal type
 */
domain::MoveData CreateSwordsDance();

/**
 * @brief Create the Iron Defense move data
 * Gen III: 0 power, self-targeting, Steel type
 */
domain::MoveData CreateIronDefense();

/**
 * @brief Create the Agility move data
 * Gen III: 0 power, self-targeting, Psychic type
 */
domain::MoveData CreateAgility();

/**
 * @brief Create the Tail Glow move data
 * Gen III: 0 power, self-targeting, Bug type
 */
domain::MoveData CreateTailGlow();

/**
 * @brief Create the Fake Tears move data
 * Gen III: 0 power, 100 accuracy, Dark type
 */
domain::MoveData CreateFakeTears();

/**
 * @brief Create the Amnesia move data
 * Gen III: 0 power, self-targeting, Psychic type
 */
domain::MoveData CreateAmnesia();

/**
 * @brief Create the String Shot move data
 * Gen III: 0 power, 95 accuracy, Bug type
 */
domain::MoveData CreateStringShot();

/**
 * @brief Create the Double-Edge move data
 * Gen III: 120 power, 100 accuracy, Normal type, 33% recoil
 */
domain::MoveData CreateDoubleEdge();

/**
 * @brief Create the Giga Drain move data
 * Gen III: 60 power, 100 accuracy, Grass type, 50% drain
 */
domain::MoveData CreateGigaDrain();

/**
 * @brief Create the Fury Attack move data
 * Gen III: 15 power, 85 accuracy, Normal type, hits 2-5 times
 */
domain::MoveData CreateFuryAttack();

/**
 * @brief Create the Protect move data
 * Gen III: 0 power, self-targeting, Normal type, +4 priority
 */
domain::MoveData CreateProtect();

/**
 * @brief Create the Solar Beam move data
 * Gen III: 120 power, 100 accuracy, Grass type, two-turn move
 */
domain::MoveData CreateSolarBeam();

/**
 * @brief Create the Fly move data
 * Gen III: 70 power, 95 accuracy, Flying type, two-turn semi-invulnerable
 */
domain::MoveData CreateFly();

/**
 * @brief Create the Substitute move data
 * Gen III: 0 power, self-targeting, Normal type, creates substitute at 25% HP
 */
domain::MoveData CreateSubstitute();

/**
 * @brief Create the Baton Pass move data
 * Gen III: 0 power, self-targeting, Normal type, transfers stat stages
 */
domain::MoveData CreateBatonPass();

}  // namespace helpers
}  // namespace test
