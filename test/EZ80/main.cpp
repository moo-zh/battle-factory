/**
 * @file main.cpp
 * @brief Main entry point for the test suite.
 */

#include "framework.hpp"

/**
 * @brief Main entry point for the test suite.
 * @return 0 if all tests pass, 1 otherwise.
 */
int main() {
    CLEAR_CONSOLE();

    test::printTestHeader("  POKEMON BATTLE FACTORY -- TEST SUITE");

    test::runAllTests();
    test::printSummary();

    PRINT("\n✗ Press any key to exit...\n");
    WAIT_KEY();

    return test::g_stats.failed == 0 ? 0 : 1;
}
