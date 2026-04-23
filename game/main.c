/*
 * main.c — Phase 2 game loop.
 * Loads map (ported from ARCHIVE_pre_refactor/map.h), draws walls/floor/doors
 * + static entity spawns (enemies, gold, potions, weapons), player moves
 * with wall collision. No AI or pickup logic yet — Phase 3.
 */
#include <stdint.h>
#include "../platform/platform.h"
#include "map.h"

static uint8_t player_x, player_y;
static glyph_t tile_under_player;   /* what was on the floor we stepped onto */

static uint8_t colour_for_glyph(glyph_t g) {
    switch (g) {
        case G_WALL:   return COL_WHITE;
        case G_DOOR:   return COL_YELLOW;
        case G_FLOOR:  return COL_BLUE;
        case G_ENEMY:  return COL_RED;
        case G_GOLD:   return COL_YELLOW;
        case G_POTION: return COL_MAGENTA;
        case G_WEAPON: return COL_CYAN;
        case G_STAIRS: return COL_GREEN;
        case G_CORPSE: return COL_RED;
        default:       return COL_WHITE;
    }
}

static void draw_map(void) {
    uint8_t x, y;
    glyph_t g;
    plat_cls();
    for (y = 0; y < map_h; y++) {
        for (x = 0; x < map_w; x++) {
            g = map_tiles[y][x];
            plat_putc(x, y, g, colour_for_glyph(g));
        }
    }
}

static void draw_spawns(void) {
    uint8_t i;
    for (i = 0; i < map_spawn_count; i++) {
        glyph_t g = map_spawns[i].g;
        plat_putc(map_spawns[i].x, map_spawns[i].y, g, colour_for_glyph(g));
    }
}

int main(void) {
    uint8_t key, nx, ny;
    glyph_t target;

    plat_init();
    plat_seed_rand(0x1234);
    map_load(0);

    draw_map();
    draw_spawns();

    player_x = map_player_x;
    player_y = map_player_y;
    tile_under_player = G_FLOOR;
    plat_putc(player_x, player_y, G_PLAYER, COL_YELLOW);

    for (;;) {
        key = plat_key_wait();
        nx = player_x;
        ny = player_y;
        switch (key) {
            case K_UP:    if (player_y > 0)         ny = player_y - 1; break;
            case K_DOWN:  if (player_y < map_h - 1) ny = player_y + 1; break;
            case K_LEFT:  if (player_x > 0)         nx = player_x - 1; break;
            case K_RIGHT: if (player_x < map_w - 1) nx = player_x + 1; break;
            case K_QUIT:  plat_shutdown(); return 0;
            default: continue;
        }
        if (nx == player_x && ny == player_y) continue;

        target = map_get(nx, ny);
        if (map_is_solid(target)) continue;

        /* Restore tile we stepped off. */
        plat_putc(player_x, player_y, tile_under_player,
                  colour_for_glyph(tile_under_player));
        tile_under_player = target;
        player_x = nx;
        player_y = ny;
        plat_putc(player_x, player_y, G_PLAYER, COL_YELLOW);
    }
}
