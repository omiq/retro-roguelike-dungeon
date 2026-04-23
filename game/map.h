/*
 * map.h — platform-neutral map storage + accessors.
 * Map size is set at init from plat_screen_w()/plat_screen_h(). Room
 * templates are ASCII strings (see map.c); map_load() converts them
 * into glyph_t tiles so the rest of the game never sees raw chars.
 */
#ifndef GAME_MAP_H
#define GAME_MAP_H

#include <stdint.h>
#include "glyphs.h"

#define MAP_MAX_W 40
#define MAP_MAX_H 25

extern uint8_t map_w;
extern uint8_t map_h;
extern glyph_t map_tiles[MAP_MAX_H][MAP_MAX_W];

/* Entity spawns parsed out of the template (player start, enemies, gold...). */
#define MAP_MAX_SPAWNS 32
typedef struct {
    uint8_t x, y;
    glyph_t g;
} map_spawn_t;
extern map_spawn_t map_spawns[MAP_MAX_SPAWNS];
extern uint8_t     map_spawn_count;
extern uint8_t     map_player_x, map_player_y;   /* from '@' marker */

void    map_load(uint8_t room_index);
glyph_t map_get(uint8_t x, uint8_t y);
uint8_t map_is_solid(glyph_t g);

#endif
