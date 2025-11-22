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
# Custom Build Targets
# ----------------------------

.PHONY: utest

# Unit test target - build test harness from test/ directory with debug flags
# Structure: test/main.cpp, test/unit/
utest:
	@$(MAKE) debug NAME=UTEST SRCDIR=test/ DESCRIPTION="Test Harness" \
		CXXFLAGS="$(CXXFLAGS) -I$(SRCDIR)" \
		EXTRA_CXX_SOURCES="$(wildcard $(SRCDIR)**/*.cpp)"