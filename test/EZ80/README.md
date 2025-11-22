# TI-84 CE Test Framework

Lightweight test framework for TI-84 Plus CE calculator programs.

## Purpose

This framework enables hardware-specific testing for:
- **UI rendering** - Test graphical interfaces on actual screen
- **Input handling** - Test keypad input
- **Asset loading** - Test compressed text/graphics decompression
- **Performance** - Measure frame times on real hardware
- **Memory constraints** - Validate RAM/ROM usage

## Features

- Auto-registration of tests via constructor functions
- Colored output via CEmu debug console
- Assertion macros (`TEST_ASSERT`, `TEST_ASSERT_EQ`, etc.)
- Batch testing for probabilistic tests
- Setup/teardown support
- Summary reporting

## Usage

### Basic Test

```cpp
#include "test/ce/framework.hpp"

TEST_CASE(MyTest) {
    int result = 2 + 2;
    TEST_ASSERT_EQ(result, 4, "Math should work");
}
```

### Main File

```cpp
#include "test/ce/framework.hpp"

int main(void) {
    test::printTestHeader("My Test Suite");
    test::runAllTests();
    test::printSummary();

    // Wait for keypress before exiting
    while (!os_GetCSC());
    return 0;
}
```

### Build

Add to your Makefile:

```makefile
# Test configuration
TEST_CXXFLAGS = $(CXXFLAGS) -Isrc/ -Itest/
TEST_FRAMEWORK = test/ce/framework.cpp test/ce/main.cpp
TEST_SOURCES = $(wildcard src/**/*.cpp)

test-ui:
	@$(MAKE) debug NAME=UITEST SRCDIR=test/ce/ui/ \
		DESCRIPTION="UI Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"
```

## API Reference

### Test Registration

```cpp
TEST_CASE(test_name) {
    // Test body
}
```

### Assertions

```cpp
TEST_ASSERT(condition, message)
TEST_ASSERT_EQ(actual, expected, message)
TEST_ASSERT_NE(actual, not_expected, message)
TEST_ASSERT_NOT_NULL(ptr, message)
TEST_ASSERT_NULL(ptr, message)
TEST_ASSERT_BATCH(condition, message, iterations)
```

### Setup/Teardown

```cpp
TEST_SETUP() {
    // Runs before each test
}

TEST_TEARDOWN() {
    // Runs after each test
}
```

### Output

```cpp
test::printTestHeader("Suite Name");
test::runAllTests();
test::printSummary();
test::resetStats();  // For multiple test suites
```

## Example: UI Test

```cpp
#include "test/ce/framework.hpp"
#include "ui/menu.hpp"
#include <graphx.h>

TEST_SETUP() {
    gfx_Begin();
    gfx_SetDrawBuffer();
}

TEST_TEARDOWN() {
    gfx_End();
}

TEST_CASE(Menu_Renders) {
    ui::Menu menu;
    menu.AddItem("Option 1");
    menu.AddItem("Option 2");

    menu.Render();
    gfx_SwapDraw();

    // Visual verification on CEmu screen
    TEST_ASSERT(true, "Menu rendered without crash");
}

int main(void) {
    test::printTestHeader("UI Tests");
    test::runAllTests();
    test::printSummary();

    while (!os_GetCSC());
    return 0;
}
```

## Running Tests

### On CEmu

```bash
make test-ui
cemu-cli --run build/UITEST.8xp
```

### Debugging

- Use `dbg_printf()` for debug output (appears in CEmu console)
- Set breakpoints in CEmu debugger
- Check memory usage in CEmu stats

## Limitations

### RAM Constraints

Each test binary is limited by available RAM (~154 KB user-accessible). If tests exceed capacity:

1. Split into multiple test binaries (e.g., `test-ui-1`, `test-ui-2`)
2. Use `COMPRESSED=YES` in Makefile (trades ROM for RAM)
3. Reduce test data sizes

### No Dynamic Discovery

Tests are registered at compile-time via constructors. Maximum 256 tests per binary (configurable in `framework.hpp`).

## When to Use CE Tests vs GTest

**Use CE framework when:**
- Testing hardware-specific features (UI, graphics, input)
- Validating performance on actual hardware
- Testing memory constraints
- Visual verification needed

**Use GTest when:**
- Testing platform-agnostic logic (battle engine)
- Running large test suites (>500 tests)
- CI/CD integration
- Faster iteration during development

## Files

```
test/ce/
├── framework.hpp     # Test framework interface + macros
├── framework.cpp     # Framework implementation
├── main.cpp          # Default test runner
└── README.md         # This file
```

## History

This framework was originally developed for the concept ladder validation (486 tests). It has been preserved for future hardware-specific testing.

See `test/archived/` for historical test examples.
