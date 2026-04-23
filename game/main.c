/*
 * main.c — Phase 3.1 game loop, incremental redraw.
 * Full map drawn once at init. Each turn only redraws:
 *   - player old/new cell
 *   - killed enemy -> corpse
 *   - pickup -> floor restore
 *   - each enemy that moved -> old cell (restore underlying map tile) + new cell
 *   - status bar (only when stats changed)
 * Everything else left alone, so flicker vanishes on slow machines.
 */
#include <stdint.h>
#include "../platform/platform.h"
#include "map.h"
#include "entity.h"

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

/* Status bar drawn on the row immediately below the map. */
static char status_buf[41];
static uint8_t prev_hp, prev_gold, prev_dmg;

static void u8_to_str(uint8_t v, char *out) {
    uint8_t h = v / 100, t = (v / 10) % 10, o = v % 10, p = 0;
    if (h)      out[p++] = '0' + h;
    if (h || t) out[p++] = '0' + t;
    out[p++] = '0' + o;
    out[p] = '\0';
}

static void build_status(void) {
    uint8_t i;
    char tmp[4];
    for (i = 0; i < 40; i++) status_buf[i] = ' ';
    status_buf[40] = '\0';
    status_buf[0] = 'H'; status_buf[1] = 'P'; status_buf[2] = ':';
    u8_to_str(player_hp, tmp);
    for (i = 0; tmp[i]; i++) status_buf[3 + i] = tmp[i];
    status_buf[9]  = 'G'; status_buf[10] = 'L'; status_buf[11] = 'D'; status_buf[12] = ':';
    u8_to_str(player_gold, tmp);
    for (i = 0; tmp[i]; i++) status_buf[13 + i] = tmp[i];
    status_buf[19] = 'D'; status_buf[20] = 'M'; status_buf[21] = 'G'; status_buf[22] = ':';
    u8_to_str(player_dmg, tmp);
    for (i = 0; tmp[i]; i++) status_buf[23 + i] = tmp[i];
}

static void redraw_status_if_changed(void) {
    if (player_hp == prev_hp && player_gold == prev_gold && player_dmg == prev_dmg)
        return;
    build_status();
    plat_puts(0, map_h, status_buf, COL_CYAN);
    prev_hp = player_hp; prev_gold = player_gold; prev_dmg = player_dmg;
}

/* Redraw one cell from map + any live entity sitting on it. */
static void redraw_cell(uint8_t x, uint8_t y) {
    int8_t ei = entity_at(x, y);
    glyph_t g;
    if (ei >= 0 && entities[ei].alive) {
        g = entities[ei].g;
    } else {
        g = map_get(x, y);
    }
    plat_putc(x, y, g, colour_for_glyph(g));
}

static void initial_render(uint8_t px, uint8_t py) {
    uint8_t x, y, i;
    glyph_t g;
    plat_cls();
    for (y = 0; y < map_h; y++) {
        for (x = 0; x < map_w; x++) {
            g = map_tiles[y][x];
            plat_putc(x, y, g, colour_for_glyph(g));
        }
    }
    for (i = 0; i < entity_count; i++) {
        if (!entities[i].alive) continue;
        plat_putc(entities[i].x, entities[i].y, entities[i].g,
                  colour_for_glyph(entities[i].g));
    }
    plat_putc(px, py, G_PLAYER, COL_YELLOW);
    /* Force status draw on first render regardless of diff. */
    prev_hp = player_hp + 1;
    redraw_status_if_changed();
}

/* Resolve the player's attempt to enter (nx,ny). Returns 1 if player moved.
 * Patches affected cells (target if enemy killed or item picked up). */
static uint8_t step_onto(uint8_t *px, uint8_t *py, uint8_t nx, uint8_t ny) {
    int8_t ei;
    glyph_t target = map_get(nx, ny);
    if (map_is_solid(target)) return 0;
    ei = entity_at(nx, ny);
    if (ei >= 0) {
        entity_t *e = &entities[ei];
        if (e->g == G_ENEMY) {
            e->hp -= (int8_t)player_dmg;
            if (e->hp <= 0) {
                entity_kill((uint8_t)ei);
                /* corpse glyph now on map; redraw cell. */
                plat_putc(nx, ny, G_CORPSE, colour_for_glyph(G_CORPSE));
            }
            return 0;
        }
        /* Pickup. */
        switch (e->g) {
            case G_GOLD:   player_gold++;                       break;
            case G_POTION: if (player_hp < 250) player_hp += 3; break;
            case G_WEAPON: player_dmg++;                        break;
            default: break;
        }
        e->alive = 0;
        /* Item sprite gone; show underlying map tile briefly before player lands. */
        plat_putc(nx, ny, map_get(nx, ny), colour_for_glyph(map_get(nx, ny)));
    }
    *px = nx;
    *py = ny;
    return 1;
}

int main(void) {
    uint8_t key, nx, ny, px, py;
    uint8_t hit, i;

    plat_init();
    plat_seed_rand(0x1234);
    map_load(0);
    entity_init_from_map_spawns();

    px = map_player_x;
    py = map_player_y;
    initial_render(px, py);

    for (;;) {
        uint8_t moved;
        uint8_t old_px = px, old_py = py;

        key = plat_key_wait();
        nx = px; ny = py;
        switch (key) {
            case K_UP:    if (py > 0)         ny = py - 1; break;
            case K_DOWN:  if (py < map_h - 1) ny = py + 1; break;
            case K_LEFT:  if (px > 0)         nx = px - 1; break;
            case K_RIGHT: if (px < map_w - 1) nx = px + 1; break;
            case K_QUIT:  plat_shutdown(); return 0;
            default: continue;
        }
        moved = 0;
        if (nx != px || ny != py) moved = step_onto(&px, &py, nx, ny);
        if (moved) {
            /* Restore cell player left. */
            redraw_cell(old_px, old_py);
            /* Draw player at new cell. */
            plat_putc(px, py, G_PLAYER, COL_YELLOW);
        }

        /* Enemy turn — fills move_events. */
        entity_ai_turn(px, py);
        for (i = 0; i < move_event_count; i++) {
            /* old cell: show whatever map/other entity is there now. */
            redraw_cell(move_events[i].ox, move_events[i].oy);
            /* new cell: draw this enemy on top. */
            plat_putc(move_events[i].nx, move_events[i].ny,
                      move_events[i].g, colour_for_glyph(move_events[i].g));
        }

        /* Adjacent enemies damage player. */
        hit = entity_adjacent_damage(px, py);
        if (hit >= player_hp) {
            player_hp = 0;
            redraw_status_if_changed();
            plat_puts(0, 0, "YOU DIED - press Q", COL_RED);
            for (;;) if (plat_key_wait() == K_QUIT) { plat_shutdown(); return 0; }
        } else {
            player_hp -= hit;
        }
        redraw_status_if_changed();
    }
}
