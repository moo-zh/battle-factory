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
# TI-84 CE tests have been archived to test/EZ80/archived/ (source code reference only)
# Use CMake for host-based GTest testing:
#   mkdir build-host && cd build-host
#   cmake .. && make && ctest