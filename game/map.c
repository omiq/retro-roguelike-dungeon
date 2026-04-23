/*
 * map.c — room templates + loader.
 * Ported from ARCHIVE_pre_refactor/map.h (hand-drawn 40×24 rooms).
 * Templates stay as ASCII so they remain readable; load converts to glyph_t.
 *
 * ASCII legend (archive convention):
 *   '.' floor, '#' wall, '+' door, '@' player start,
 *   'K' key, 'G' goblin-ish enemy, 'R' rat enemy, 'H' enemy,
 *   'I' item, 'P' potion, '$' gold, '/' weapon, '%' corpse
 * Unknown chars default to G_FLOOR.
 */
#include "map.h"
#include "enemy_types.h"
#include "../platform/platform.h"

uint8_t       map_w;
uint8_t       map_h;
glyph_t       map_tiles[MAP_MAX_H][MAP_MAX_W];
map_spawn_t   map_spawns[MAP_MAX_SPAWNS];
uint8_t       map_spawn_count;
uint8_t       map_player_x;
uint8_t       map_player_y;

/* Room 0 — 40 wide × 24 tall. Lines shorter than map_w pad with floor. */
#define ROOM_W 40
#define ROOM_H 24
#define ROOM_COUNT 1

static const char * const rooms[ROOM_COUNT][ROOM_H] = {
    {
        "........................................",
        "........................................",
        "........................................",
        ".#################################......",
        ".#...................#...........#......",
        ".#.@.....K...G.......#...R.......#......",
        ".#...................+...........#......",
        ".#....P..........G...#.......I...#......",
        ".#.H.................#...........#......",
        ".#...................#...........#......",
        ".#.........#####+#########.....H.#......",
        ".###########.............#.......#......",
        ".#.........#.............#.......#......",
        ".#.........#..$..........#...K...#......",
        ".#.....H...#.............#.......#......",
        ".#...K.....+.............#.......#......",
        ".#.........#....../.I....+....G..#......",
        ".#.........#.............#.......#......",
        ".#....R....#..R..........#.......#......",
        ".#.........#.............#.......#......",
        ".#.........#.............#..$....#......",
        ".###########.............#.......#......",
        "..........########################......",
        "........................................"
    }
};

static glyph_t ascii_to_glyph(char c, uint8_t *is_spawn, glyph_t *spawn_g,
                              int8_t *type_idx) {
    *is_spawn = 0;
    *type_idx = -1;
    switch (c) {
        case '#': return G_WALL;
        case '+': return G_DOOR;
        case '.': return G_FLOOR;
        case ' ': return G_SPACE;
        case '@': *is_spawn = 1; *spawn_g = G_PLAYER; return G_FLOOR;
        case 'K': case 'G': case 'R':
                  *is_spawn = 1; *spawn_g = G_ENEMY;
                  *type_idx = enemy_type_from_marker(c);
                  return G_FLOOR;
        case 'H': *is_spawn = 1; *spawn_g = G_HEALTH; return G_FLOOR;
        case 'P': *is_spawn = 1; *spawn_g = G_MAGIC;  return G_FLOOR;
        case 'I': *is_spawn = 1; *spawn_g = G_IDOL;   return G_FLOOR;
        case '$': *is_spawn = 1; *spawn_g = G_GOLD;   return G_FLOOR;
        case '/': *is_spawn = 1; *spawn_g = G_WEAPON; return G_FLOOR;
        case '%': return G_CORPSE;
        case '>': return G_STAIRS;
        default:  return G_FLOOR;
    }
}

void map_load(uint8_t room_index) {
    uint8_t x, y;
    const char *row;
    uint8_t sw = plat_screen_w();
    uint8_t sh = plat_screen_h();

    if (room_index >= ROOM_COUNT) room_index = 0;

    map_w = (sw < ROOM_W) ? sw : ROOM_W;
    map_h = (sh < ROOM_H) ? sh : ROOM_H;
    if (map_w > MAP_MAX_W) map_w = MAP_MAX_W;
    if (map_h > MAP_MAX_H) map_h = MAP_MAX_H;

    map_spawn_count = 0;
    map_player_x = map_w / 2;
    map_player_y = map_h / 2;

    for (y = 0; y < map_h; y++) {
        row = rooms[room_index][y];
        for (x = 0; x < map_w; x++) {
            uint8_t is_spawn;
            glyph_t sg = 0;
            int8_t  ti = -1;
            char ch = row[x];
            if (ch == '\0') { map_tiles[y][x] = G_FLOOR; continue; }
            map_tiles[y][x] = ascii_to_glyph(ch, &is_spawn, &sg, &ti);
            if (is_spawn) {
                if (sg == G_PLAYER) {
                    map_player_x = x;
                    map_player_y = y;
                } else if (map_spawn_count < MAP_MAX_SPAWNS) {
                    map_spawns[map_spawn_count].x = x;
                    map_spawns[map_spawn_count].y = y;
                    map_spawns[map_spawn_count].g = sg;
                    map_spawns[map_spawn_count].type_idx = ti;
                    map_spawn_count++;
                }
            }
        }
    }
}

glyph_t map_get(uint8_t x, uint8_t y) {
    if (x >= map_w || y >= map_h) return G_WALL;
    return map_tiles[y][x];
}

void map_set(uint8_t x, uint8_t y, glyph_t g) {
    if (x >= map_w || y >= map_h) return;
    map_tiles[y][x] = g;
}

uint8_t map_is_solid(glyph_t g) {
    return (g == G_WALL || g == G_BORDER || g == G_DOOR) ? 1 : 0;
}
