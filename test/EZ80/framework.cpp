/**
 * @file framework.cpp
 * @brief Implementation of global variables for the test framework.
 */

#include "framework.hpp"

namespace test {

/** @brief Global test statistics instance. */
TestStats g_stats = {};

/** @brief Head of the registered tests linked list. */
TestRegistration* g_test_head = nullptr;

/** @brief Global setup function pointer. */
SetupFunction g_setup = nullptr;

/** @brief Global teardown function pointer. */
TeardownFunction g_teardown = nullptr;

}  // namespace test
