# ----------------------------
# Makefile Options
# ----------------------------

NAME = PokeBatl
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
