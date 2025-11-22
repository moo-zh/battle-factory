# ----------------------------
# Makefile Options
# ----------------------------

NAME = POKEMON
ICON = icon.png
DESCRIPTION = "Pokemon Battle Factory"

CXXFLAGS = -Wall -Wextra -Werror -Oz -std=c++17

SRCDIR = src/
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

# Common test configuration (shared by all test targets)
TEST_CXXFLAGS = $(CXXFLAGS) -Isrc/ -Itest/
TEST_FRAMEWORK = test/framework.cpp test/main.cpp
TEST_SOURCES = $(wildcard src/**/*.cpp)

.PHONY: utest utest-foundation utest-status itest tests

# Foundation tests (concept ladder - moves that validated architecture)
utest-foundation:
	@$(MAKE) debug NAME=UTFOUND SRCDIR=test/unit/foundation/ \
		DESCRIPTION="Foundation Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"

# Status condition tests (comprehensive status mechanics testing)
utest-status:
	@$(MAKE) debug NAME=UTSTAT SRCDIR=test/unit/status/ \
		DESCRIPTION="Status Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"

# Run all unit tests
utest: utest-foundation utest-status

# Integration test target - build test harness for engine integration
# Structure: test/main.cpp, test/integration/ (full engine tests)
itest:
	@$(MAKE) debug NAME=ITEST SRCDIR=test/integration/ DESCRIPTION="Integration Tests" \
		COMPRESSED=NO \
		CXXFLAGS="$(TEST_CXXFLAGS)" \
		EXTRA_CXX_SOURCES="$(TEST_FRAMEWORK) $(TEST_SOURCES)"

# Build all tests - builds both unit and integration tests
tests: utest itest