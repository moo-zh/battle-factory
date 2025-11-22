/**
 * @file test/host/test_common.hpp
 * @brief Convenience header for GTest unit tests
 *
 * This header includes all common test dependencies to reduce boilerplate
 * and make tests more resilient to directory reorganization.
 *
 * Usage:
 *   #include <gtest/gtest.h>
 *   #include "test_common.hpp"
 *
 * Instead of:
 *   #include <gtest/gtest.h>
 *   #include "battle/effects/basic.hpp"
 *   #include "battle/random.hpp"
 *   #include "battle_helpers.hpp"
 *   #include "pokemon_factory.hpp"
 */

#pragma once

// ============================================================================
// Test Helpers
// ============================================================================

#include "battle_helpers.hpp"   // CreateBattleContext(), CreateTackle(), etc.
#include "pokemon_factory.hpp"  // CreateCharmander(), CreateBulbasaur(), etc.

// ============================================================================
// Battle Engine (commonly used in tests)
// ============================================================================

#include "battle/context.hpp"
#include "battle/effects/basic.hpp"
#include "battle/random.hpp"
#include "battle/state/pokemon.hpp"

// ============================================================================
// Domain Types (commonly referenced)
// ============================================================================

#include "domain/move.hpp"
#include "domain/species.hpp"
#include "domain/stats.hpp"

// ============================================================================
// Convenience Namespace
// ============================================================================

// Bring test helpers into scope for convenience
using namespace test::helpers;
