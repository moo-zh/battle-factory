# ----------------------------
# Makefile Options
# ----------------------------

NAME = POKEMON
ICON = icon.png
DESCRIPTION = "Pokemon Battle Factory"

CXXFLAGS = -Wall -Wextra -Werror -Oz -std=c++17

SRCDIR = source/
BINDIR = build/
OBJDIR = build/object/

COMPRESSED = YES
COMPRESSED_MODE = zx0

ARCHIVED = YES

# ----------------------------

include $(shell cedev-config --makefile)

# ----------------------------
# Test Configuration
# ----------------------------

# Common test configuration (shared by utest and itest)
TEST_CXXFLAGS = $(CXXFLAGS) -Isource/ -Itest/
TEST_FRAMEWORK = test/framework.cpp test/main.cpp
TEST_SOURCES = $(wildcard source/**/*.cpp)

.PHONY: utest itest tests

# Unit test target - build test harness for move effects
# Structure: test/main.cpp, test/unit/ (effect unit tests)
utest:
	@$(MAKE) debug NAME=UTEST SRCDIR=test/unit/ DESCRIPTION="Unit Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"

# Integration test target - build test harness for engine integration
# Structure: test/main.cpp, test/integration/ (full engine tests)
itest:
	@$(MAKE) debug NAME=ITEST SRCDIR=test/integration/ DESCRIPTION="Integration Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"

# Run all tests - builds both unit and integration tests
tests: utest itest