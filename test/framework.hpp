/**
 * @file framework.hpp
 * @brief Lightweight test framework for TI-84+ CE calculator programs.
 *
 * This framework provides test registration, assertion macros, setup/teardown
 * support, and colored output for debugging via the CEmu emulator console.
 */

#pragma once

#include <debug.h>
#include <stdint.h>
#include <ti/getcsc.h>

/**
 * @namespace test
 * @brief Test framework namespace containing all testing utilities.
 */
namespace test {

/**
 * @namespace config
 * @brief Configuration constants for the test framework.
 */
namespace config {
/** @brief Maximum number of tests that can be registered. */
constexpr uint8_t MAX_TESTS = 64;
}  // namespace config

/**
 * @struct TestStats
 * @brief Tracks test execution statistics.
 */
struct TestStats {
    uint16_t passed = 0;  ///< Number of passed assertions
    uint16_t failed = 0;  ///< Number of failed assertions
    uint16_t total = 0;   ///< Total number of assertions executed
};

/** @brief Global test statistics instance. */
extern TestStats g_stats;

/**
 * @namespace Color
 * @brief ANSI color codes for TI-84+ CE debug output.
 */
namespace Color {
constexpr const char* RESET = "\x1b[0m";     ///< Reset to default color
constexpr const char* RED = "\x1b[31m";      ///< Red text
constexpr const char* GREEN = "\x1b[32m";    ///< Green text
constexpr const char* YELLOW = "\x1b[33m";   ///< Yellow text
constexpr const char* BLUE = "\x1b[34m";     ///< Blue text
constexpr const char* MAGENTA = "\x1b[35m";  ///< Magenta text
constexpr const char* CYAN = "\x1b[36m";     ///< Cyan text
constexpr const char* WHITE = "\x1b[37m";    ///< White text
}  // namespace Color

/** @brief Clear the debug console. */
#define CLEAR_CONSOLE() dbg_ClearConsole();

/** @brief Print formatted output to debug console. */
#define PRINT(...) dbg_printf(__VA_ARGS__)

/** @brief Wait for a keypress before continuing. */
#define WAIT_KEY() while (!os_GetCSC())

/**
 * @defgroup assertions Test Assertion Macros
 * @brief Macros for asserting conditions in tests.
 * @{
 */

/**
 * @brief Assert that a condition is true.
 * @param condition The condition to evaluate.
 * @param message Description of what is being tested.
 */
#define TEST_ASSERT(condition, message)                                                            \
    do {                                                                                           \
        test::g_stats.total++;                                                                     \
        if (condition) {                                                                           \
            test::g_stats.passed++;                                                                \
            dbg_printf("  %s[PASS]%s %s\n", test::Color::GREEN, test::Color::RESET, message);      \
        } else {                                                                                   \
            test::g_stats.failed++;                                                                \
            dbg_printf("  %s[FAIL]%s %s (%s:%d)\n", test::Color::RED, test::Color::RESET, message, \
                       __FILE__, __LINE__);                                                        \
        }                                                                                          \
    } while (0)

/**
 * @brief Assert that two values are equal.
 * @param actual The actual value.
 * @param expected The expected value.
 * @param message Description of what is being tested.
 */
#define TEST_ASSERT_EQ(actual, expected, message)                                             \
    do {                                                                                      \
        auto _test_actual = (actual);                                                         \
        auto _test_expected = (expected);                                                     \
        test::g_stats.total++;                                                                \
        if (_test_actual == _test_expected) {                                                 \
            test::g_stats.passed++;                                                           \
            dbg_printf("  %s[PASS]%s %s\n", test::Color::GREEN, test::Color::RESET, message); \
        } else {                                                                              \
            test::g_stats.failed++;                                                           \
            dbg_printf("  %s[FAIL]%s %s (got: %d, expected: %d) (%s:%d)\n", test::Color::RED, \
                       test::Color::RESET, message, (int)_test_actual, (int)_test_expected,   \
                       __FILE__, __LINE__);                                                   \
        }                                                                                     \
    } while (0)

/**
 * @brief Assert that two values are not equal.
 * @param actual The actual value.
 * @param expected The value that actual should not equal.
 * @param message Description of what is being tested.
 */
#define TEST_ASSERT_NE(actual, expected, message)                                             \
    do {                                                                                      \
        auto _test_actual = (actual);                                                         \
        auto _test_expected = (expected);                                                     \
        test::g_stats.total++;                                                                \
        if (_test_actual != _test_expected) {                                                 \
            test::g_stats.passed++;                                                           \
            dbg_printf("  %s[PASS]%s %s\n", test::Color::GREEN, test::Color::RESET, message); \
        } else {                                                                              \
            test::g_stats.failed++;                                                           \
            dbg_printf("  %s[FAIL]%s %s (both were: %d) (%s:%d)\n", test::Color::RED,         \
                       test::Color::RESET, message, (int)_test_actual, __FILE__, __LINE__);   \
        }                                                                                     \
    } while (0)

/**
 * @brief Assert that a pointer is not null.
 * @param ptr The pointer to check.
 * @param message Description of what is being tested.
 */
#define TEST_ASSERT_NOT_NULL(ptr, message)                                                    \
    do {                                                                                      \
        auto _test_ptr = (ptr);                                                               \
        test::g_stats.total++;                                                                \
        if (_test_ptr != nullptr) {                                                           \
            test::g_stats.passed++;                                                           \
            dbg_printf("  %s[PASS]%s %s\n", test::Color::GREEN, test::Color::RESET, message); \
        } else {                                                                              \
            test::g_stats.failed++;                                                           \
            dbg_printf("  %s[FAIL]%s %s (was null) (%s:%d)\n", test::Color::RED,              \
                       test::Color::RESET, message, __FILE__, __LINE__);                      \
        }                                                                                     \
    } while (0)

/**
 * @brief Assert that a pointer is null.
 * @param ptr The pointer to check.
 * @param message Description of what is being tested.
 */
#define TEST_ASSERT_NULL(ptr, message)                                                        \
    do {                                                                                      \
        auto _test_ptr = (ptr);                                                               \
        test::g_stats.total++;                                                                \
        if (_test_ptr == nullptr) {                                                           \
            test::g_stats.passed++;                                                           \
            dbg_printf("  %s[PASS]%s %s\n", test::Color::GREEN, test::Color::RESET, message); \
        } else {                                                                              \
            test::g_stats.failed++;                                                           \
            dbg_printf("  %s[FAIL]%s %s (was not null) (%s:%d)\n", test::Color::RED,          \
                       test::Color::RESET, message, __FILE__, __LINE__);                      \
        }                                                                                     \
    } while (0)

/** @} */  // end of assertions group

/**
 * @typedef SetupFunction
 * @brief Function pointer type for test setup functions.
 * @note Only one global setup is supported per test binary.
 */
using SetupFunction = void (*)();

/**
 * @typedef TeardownFunction
 * @brief Function pointer type for test teardown functions.
 * @note Only one global teardown is supported per test binary.
 */
using TeardownFunction = void (*)();

/** @brief Global setup function pointer. */
extern SetupFunction g_setup;

/** @brief Global teardown function pointer. */
extern TeardownFunction g_teardown;

/**
 * @brief Define a setup function to run before each test.
 *
 * Usage:
 * @code
 * TEST_SETUP() {
 *     // initialization code
 * }
 * @endcode
 */
#define TEST_SETUP()                                            \
    static void test_setup_func();                              \
    static void __attribute__((constructor)) register_setup() { \
        test::g_setup = test_setup_func;                        \
    }                                                           \
    static void test_setup_func()

/**
 * @brief Define a teardown function to run after each test.
 *
 * Usage:
 * @code
 * TEST_TEARDOWN() {
 *     // cleanup code
 * }
 * @endcode
 */
#define TEST_TEARDOWN()                                            \
    static void test_teardown_func();                              \
    static void __attribute__((constructor)) register_teardown() { \
        test::g_teardown = test_teardown_func;                     \
    }                                                              \
    static void test_teardown_func()

/**
 * @brief Define a test case that is automatically registered.
 * @param test_name The name of the test function.
 *
 * Usage:
 * @code
 * TEST_CASE(my_test) {
 *     TEST_ASSERT(true, "Basic assertion");
 * }
 * @endcode
 */
#define TEST_CASE(test_name)                                                   \
    void test_name();                                                          \
    namespace {                                                                \
    struct test_name##_registrar {                                             \
        test_name##_registrar() { test::registerTest(#test_name, test_name); } \
    };                                                                         \
    static test_name##_registrar test_name##_registrar_instance;               \
    }                                                                          \
    void test_name()

/**
 * @typedef TestFunction
 * @brief Function pointer type for test functions.
 */
using TestFunction = void (*)();

/**
 * @struct TestRegistration
 * @brief Linked list node for registered tests.
 */
struct TestRegistration {
    const char* name;        ///< Test name
    TestFunction func;       ///< Test function pointer
    TestRegistration* next;  ///< Next test in the list
};

/** @brief Head of the registered tests linked list. */
extern TestRegistration* g_test_head;

/**
 * @brief Register a test function with the framework.
 * @param name The test name.
 * @param func The test function pointer.
 */
inline void registerTest(const char* name, TestFunction func) {
    static TestRegistration registrations[config::MAX_TESTS];
    static uint8_t registration_count = 0;

    if (registration_count >= config::MAX_TESTS) {
        dbg_printf("%s[ERROR] Test limit (%d) exceeded!%s\n", Color::RED, config::MAX_TESTS,
                   Color::RESET);
        return;
    }

    TestRegistration* reg = &registrations[registration_count++];
    reg->name = name;
    reg->func = func;
    reg->next = g_test_head;
    g_test_head = reg;
}

/**
 * @brief Run a single test with setup/teardown.
 * @param reg Pointer to the test registration.
 */
inline void runTest(TestRegistration* reg) {
    dbg_printf("\n%s%s%s\n", Color::YELLOW, reg->name, Color::RESET);

    // Run setup if defined
    if (g_setup) {
        g_setup();
    }

    // Run test
    reg->func();

    // Run teardown if defined
    if (g_teardown) {
        g_teardown();
    }
}

/**
 * @brief Run all registered tests in declaration order.
 */
inline void runAllTests() {
    TestRegistration* current = g_test_head;

    // Debug: check if any tests registered
    if (!current) {
        dbg_printf("%sNo tests registered!%s\n", Color::RED, Color::RESET);
        return;
    }

    // Reverse the list to run tests in declaration order
    TestRegistration* prev = nullptr;
    while (current) {
        TestRegistration* next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    g_test_head = prev;

    // Run tests
    current = g_test_head;
    while (current) {
        runTest(current);
        current = current->next;
    }
}

/**
 * @brief Print a test suite header.
 * @param suite_name The name of the test suite.
 */
inline void printTestHeader(const char* suite_name) {
    dbg_printf("\n");
    dbg_printf("%s========================================%s\n", Color::CYAN, Color::RESET);
    dbg_printf("%s\n", suite_name);
    dbg_printf("%s========================================%s\n", Color::CYAN, Color::RESET);
}

/**
 * @brief Print test results summary.
 */
inline void printSummary() {
    dbg_printf("\n");
    dbg_printf("%s========================================%s\n", Color::CYAN, Color::RESET);
    dbg_printf("              TEST SUMMARY\n");
    dbg_printf("%s========================================%s\n", Color::CYAN, Color::RESET);
    dbg_printf("Total:  %u\n", g_stats.total);
    dbg_printf("%sPassed: %u%s\n", Color::GREEN, g_stats.passed, Color::RESET);

    if (g_stats.failed > 0) {
        dbg_printf("%sFailed: %u%s\n", Color::RED, g_stats.failed, Color::RESET);
    } else {
        dbg_printf("Failed: %u\n", g_stats.failed);
    }

    if (g_stats.failed == 0) {
        dbg_printf("\n%s✓ ALL TESTS PASSED%s\n", Color::GREEN, Color::RESET);
    } else {
        dbg_printf("\n%s✗ SOME TESTS FAILED%s\n", Color::RED, Color::RESET);
    }
    dbg_printf("%s========================================%s\n", Color::CYAN, Color::RESET);
}

/**
 * @brief Reset test statistics for running multiple suites.
 */
inline void resetStats() {
    g_stats.passed = 0;
    g_stats.failed = 0;
    g_stats.total = 0;
}

}  // namespace test
