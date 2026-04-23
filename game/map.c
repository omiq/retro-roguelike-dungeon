/*
 * map.c — room templates + loader.
 *
 * Role in the architecture
 * --------------------------
 * This file is the only place ASCII room art lives. Everyone else works in
 * logical glyph_t values (G_* from glyphs.h) so platform adapters never
 * parse '#' or '@' themselves. map_load() is the single conversion step:
 * it fills map_tiles[][], records non-player "game objects" into
 * map_game_objects[] (see map.h), and sets map_player_x/y from '@'.
 *
 * Why two layers (tile + game objects)?
 * --------------------------------------
 * Walkable floor is stored in map_tiles; enemies and pickups sit on floor
 * cells but are listed separately so entity_init_from_map_game_objects()
 * can build entity_t rows (HP, AI type, etc.) without overloading the
 * tile enum with every item kind.
 *
 * Sizing (map_w / map_h)
 * -----------------------
 * Templates are authored at ROOM_W × ROOM_H (40×24) but the visible map is
 * clipped to the adapter's plat_screen_w()/plat_screen_h() so the same
 * room runs on 40-column C64 and narrower VIC-20. Two bottom rows are
 * reserved for the status bar + a blank guard row (see map_load).
 *
 * ASCII legend (matches ascii_to_glyph below)
 * --------------------------------------------
 *   '.' floor, '#' wall, '+' door, '-' door (ajar / forced open), '@' player,
 *   'G'/'R'/'T'/'&' enemies (goblin, rat, thug, skeleton), 'H' health, '*' magic, 'P' potion,
 *   'I' idol, 'k' key, '$' gold, '/' weapon, '%' corpse, '>' stairs.
 * Unknown characters become G_FLOOR.
 */
#include "map.h"
#include "enemy_types.h"
#include "../platform/platform.h"

/* --- Exported map state (declared in map.h) --- */

uint8_t       map_w;
uint8_t       map_h;
glyph_t       map_tiles[MAP_MAX_H][MAP_MAX_W];
game_object_t map_game_objects[MAP_MAX_GAME_OBJECTS];
uint8_t       map_game_object_count;
uint8_t       map_player_x;
uint8_t       map_player_y;
uint8_t       map_idols;

/* Templates: ROOM_W × ROOM_H rows per room (clip to plat_screen_* − 2 rows). */
#define ROOM_W 40
#define ROOM_H 24
#define ROOM_COUNT 3

const uint8_t map_nrooms = ROOM_COUNT;

/* Static room table: one string per row. Rows shorter than map_w read NUL as
 * floor. Old archive maps used '=' for walls — use '#' here so ascii_to_glyph
 * yields G_WALL. */

static const char * const rooms[ROOM_COUNT][ROOM_H] = {
    {
        "........................................",
        "........................................",
        "........................................",
        ".#################################......",
        ".#...................#...........#......",
        ".#.@.....T...G.......#...R.......#......",
        ".#...................+...........#......",
        ".#....*..........G...#.......I...#......",
        ".#.H.................#...........#......",
        ".#...................#...........#......",
        ".#.........#####+#########.....H.#......",
        ".###########.............#.......#......",
        ".#.........#.............#.......#......",
        ".#.........#..$..........#...T...#......",
        ".#.....H...#.....k.......#.......#......",
        ".#...T.....+.............#.......#......",
        ".#.........#....../.I....+....G..#......",
        ".#.........#.............#.......#......",
        ".#....R....#..R..........#.......#......",
        ".#..P......#.............#.......#......",
        ".#.........#..........&..#..$....#......",
        ".###########.............#.......#......",
        "..........########################......",
        "........................................"
    },
    {
        "####################....................",
        "#.......####.......#....................",
        "#.......####.......#....................",
        "#.......####.......#....................",
        "#........@.........#....................",
        "#..................#######..............",
        "#..#.RRRRRRRRR.....##.....#############.",
        "#..................##...................",
        "#..................##...................",
        "#......G...........##..........I........",
        "#..........G..#....##.........H.........",
        "#.......................................",
        "#..................##...................",
        "#.........H........##..........G..#.....",
        "#...........I......##......G............",
        "#..................##...................",
        "#..................##...................",
        "#......##############..#.RRRRRRRRR......",
        "########............#...................",
        "....................#...................",
        "....................#.......####........",
        "....................#.......####........",
        "....................#.......####........",
        "....................###################."
    },
    {
        "........................................",
        "....####.######.........................",
        "...##..####....###......................",
        "...#..........#..##############.........",
        "..##..........##..............####......",
        ".##............#......IH.........##.....",
        ".#...I.........+.........G......#.#.....",
        ".#...........G.##..G...G.......##.##....",
        ".#..............##...........###...#....",
        ".#........G......##........###.....##...",
        ".##.....G.........#......#+#........#...",
        "..#...G...........##....##...........#..",
        "..##...............##.##...RRR.......#..",
        "...###..............###..RRR.........#..",
        ".....####............+...R...........#..",
        "........########.....#..R......@.....#..",
        "...............########.R............#..",
        ".....................##..............#..",
        "......................#.............#...",
        ".......................###.........##...",
        ".........................####.....##....",
        "............................######......",
        "........................................",
        "........................................"
    }
};

/* Translate one template character to a floor tile glyph and optionally
 * describe a game object that sits on that floor. When *is_game_object is
 * set, *object_g is the entity/pickup glyph and *type_idx is only meaningful
 * for foe glyphs (index into ENEMY_TYPES). Caller (map_load) copies coords into
 * map_game_objects[] except for G_PLAYER, which only updates map_player_*. 
 
 Count the idols and set map idol variable
 
 */
static glyph_t ascii_to_glyph(char c, uint8_t *is_game_object, glyph_t *object_g,
                              int8_t *type_idx) {
    *is_game_object = 0;
    *type_idx = -1;
    switch (c) {
        case '#': return G_WALL;
        case '+': return G_DOOR;
        case '-': return G_DOOR_AJAR;
        case '.': return G_FLOOR;
        case ' ': return G_SPACE;
        case '@': *is_game_object = 1; *object_g = G_PLAYER; return G_FLOOR;
        case 'G': case 'R': case 'T': case '&':
                  *is_game_object = 1;
                  *object_g = enemy_foe_glyph_for_marker(c);
                  *type_idx = enemy_type_from_marker(c);
                  return G_FLOOR;
        case 'H': *is_game_object = 1; *object_g = G_HEALTH; return G_FLOOR;
        case '*': *is_game_object = 1; *object_g = G_MAGIC;  return G_FLOOR;
        case 'P': *is_game_object = 1; *object_g = G_POTION; return G_FLOOR;
        case 'I': *is_game_object = 1; *object_g = G_IDOL;   return G_FLOOR;
        case '$': *is_game_object = 1; *object_g = G_GOLD;   return G_FLOOR;
        case '/': *is_game_object = 1; *object_g = G_WEAPON; return G_FLOOR;
        case 'k': *is_game_object = 1; *object_g = G_KEY;    return G_FLOOR;
        case '%': return G_CORPSE;
        case '>': return G_STAIRS;
        default:  return G_FLOOR;
    }
}

/* Full room parse: clears object list, optionally clamps room_index, clips
 * dimensions to screen + MAP_MAX_*, then scans every cell. Out-of-range
 * writes to map_game_objects are dropped silently if the template exceeds
 * MAP_MAX_GAME_OBJECTS (defensive cap, not expected in shipped rooms). */

void map_load(uint8_t room_index) {
    uint8_t x, y;
    const char *row;
    uint8_t sw = plat_screen_w();
    uint8_t sh = plat_screen_h();

    if (room_index >= ROOM_COUNT) room_index = 0;

    map_w = (sw < ROOM_W) ? sw : ROOM_W;
    /* Reserve 2 rows at the bottom: 1 for status, 1 blank safety row.
     * Some targets (BBC cc65 conio) scroll the screen if any char lands on
     * the final row — keeping it empty avoids the scroll. */
    if (sh >= 2) {
        uint8_t mh = sh - 2;
        map_h = (mh < ROOM_H) ? mh : ROOM_H;
    } else {
        map_h = ROOM_H;
    }
    if (map_w > MAP_MAX_W) map_w = MAP_MAX_W;
    if (map_h > MAP_MAX_H) map_h = MAP_MAX_H;

    map_game_object_count = 0;
    map_idols           = 0;
    map_player_x = map_w / 2;
    map_player_y = map_h / 2;

    for (y = 0; y < map_h; y++) {
        row = rooms[room_index][y];
        for (x = 0; x < map_w; x++) {
            uint8_t is_game_object;
            glyph_t sg = 0;
            int8_t  ti = -1;
            char ch = row[x];
            if (ch == '\0') { map_tiles[y][x] = G_FLOOR; continue; }
            map_tiles[y][x] = ascii_to_glyph(ch, &is_game_object, &sg, &ti);
            if (is_game_object) {
                if (sg == G_PLAYER) {
                    map_player_x = x;
                    map_player_y = y;
                } else if (map_game_object_count < MAP_MAX_GAME_OBJECTS) {
                    map_game_objects[map_game_object_count].x = x;
                    map_game_objects[map_game_object_count].y = y;
                    map_game_objects[map_game_object_count].g = sg;
                    map_game_objects[map_game_object_count].type_idx = ti;
                    if (sg == G_IDOL) map_idols++;
                    map_game_object_count++;
                }
            }
        }
    }
}

/* Read-only tile access. Out-of-bounds reads return G_WALL so callers that
 * forget bounds checks still get "blocked" behaviour instead of garbage. */

glyph_t map_get(uint8_t x, uint8_t y) {
    if (x >= map_w || y >= map_h) return G_WALL;
    return map_tiles[y][x];
}

/* Mutable tile write (doors opening, corpses). No-op if out of bounds. */

void map_set(uint8_t x, uint8_t y, glyph_t g) {
    if (x >= map_w || y >= map_h) return;
    map_tiles[y][x] = g;
}

/* Collision predicate shared by player movement (main), enemy AI (entity),
 * and spell termination. Anything that should block walking belongs here;
 * floor, corpses on floor, pickups, etc. are non-solid. */

uint8_t map_is_solid(glyph_t g) {
    return (g == G_WALL || g == G_BORDER || g == G_DOOR || g == G_DOOR_AJAR) ? 1
                                                                          : 0;
}
