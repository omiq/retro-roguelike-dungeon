/*
 * glyphs.h — symbolic glyph IDs. Game code uses G_*, adapters translate
 * to native screencodes / tile indices via a lookup table.
 */
#ifndef GLYPHS_H
#define GLYPHS_H

#include <stdint.h>

typedef uint8_t glyph_t;

#define G_FLOOR    0
#define G_WALL     1
#define G_DOOR     2
#define G_PLAYER   3
#define G_ENEMY    4
#define G_GOLD     5
#define G_POTION   6
#define G_WEAPON   7
#define G_STAIRS   8
#define G_CORPSE   9
#define G_SPACE   10
#define G_BORDER  11
#define G_COUNT   12

#endif
