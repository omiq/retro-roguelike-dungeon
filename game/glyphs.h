/*
 * glyphs.h — symbolic glyph IDs. Game code uses G_*, adapters translate
 * to native screencodes / tile indices via a lookup table.
 */
#ifndef GLYPHS_H
#define GLYPHS_H

#include <stdint.h>

typedef uint8_t glyph_t;

/* Logical glyph IDs. Default PETSCII/ASCII shown — see each plat_*.c
 * glyph_native[] (may use screencode on CBM). Map templates use some of
 * the same characters; see map.c ascii_to_glyph.
 * Note: G_BORDER, G_MAGIC, and G_BOLT often map to '*' in adapters; colour
 * distinguishes magic pickup vs bolt in flight on the host build.
 * Room templates (map.c): '*' = magic, 'P' = potion; on screen G_POTION
 * still uses '!' in glyph_native[] (classic potion look). */

#define G_FLOOR     0   /* '.' floor walkable */
#define G_WALL      1   /* '#' solid */
#define G_DOOR      2   /* '+' closed door */
#define G_PLAYER    3   /* '@' reserved for draw; not a map tile id */
#define G_ENEMY     4   /* 'E' generic enemy glyph on screen */
#define G_GOLD      5   /* '$' */
#define G_POTION    6   /* '!' on screen; map marker 'P' (small HP restore in main) */
#define G_WEAPON    7   /* '/' */
#define G_STAIRS    8   /* '>' */
#define G_CORPSE    9   /* '%' */
#define G_SPACE    10   /* ' ' */
#define G_BORDER   11   /* '*' map frame / unused solid accent */
#define G_HEALTH   12   /* 'h' HP pickup */
#define G_MAGIC    13   /* '*' MP pickup in room templates (map.c) */
#define G_IDOL     14   /* '&' collect all to win */
#define G_BOLT     15   /* '*' fireball in flight */
#define G_KEY      16   /* 'k' */
#define G_DOOR_AJAR 17  /* '-' ajar / forced door */
#define G_COUNT    18

#endif
