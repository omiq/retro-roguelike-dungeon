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

/* Game objects parsed from the template (enemies, pickups, …). Player '@'
 * is handled separately via map_player_x/y. */
#define MAP_MAX_GAME_OBJECTS 32
typedef struct {
    uint8_t x, y;
    glyph_t g;
    int8_t  type_idx;   /* ENEMY_TYPE_* when g is a G_FOE_* glyph, else -1 */
} game_object_t;
extern game_object_t map_game_objects[MAP_MAX_GAME_OBJECTS];
extern uint8_t       map_game_object_count;
extern uint8_t     map_player_x, map_player_y;   /* from '@' marker */
extern uint8_t       map_idols;   /* 'I' markers on this map (set in map_load) */
extern const uint8_t map_nrooms;  /* room template count (ROOM_COUNT in map.c) */

void    map_load(uint8_t room_index);
glyph_t map_get(uint8_t x, uint8_t y);
void    map_set(uint8_t x, uint8_t y, glyph_t g);
uint8_t map_is_solid(glyph_t g);

#endif
