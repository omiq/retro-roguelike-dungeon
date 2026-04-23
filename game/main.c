/*
 * main.c — Phase 3 game loop.
 * Turn-based: player moves -> resolves bump/pickup -> enemies take turn ->
 * adjacent enemies damage player. Status bar shows HP / gold / damage.
 * Q quits; game ends when player_hp reaches 0.
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

/* Status bar kept on a single reserved row below the map. */
#define STATUS_ROW (MAP_MAX_H - 1)
static char  status_buf[41];

static void u8_to_str(uint8_t v, char *out) {
    uint8_t h = v / 100;
    uint8_t t = (v / 10) % 10;
    uint8_t o = v % 10;
    uint8_t p = 0;
    if (h) out[p++] = '0' + h;
    if (h || t) out[p++] = '0' + t;
    out[p++] = '0' + o;
    out[p] = '\0';
}

static void draw_status(void) {
    uint8_t i;
    char tmp[4];
    for (i = 0; i < 40 && i < plat_screen_w(); i++) status_buf[i] = ' ';
    status_buf[40] = '\0';
    /* HP: */
    status_buf[0] = 'H'; status_buf[1] = 'P'; status_buf[2] = ':';
    u8_to_str(player_hp, tmp);
    for (i = 0; tmp[i]; i++) status_buf[3 + i] = tmp[i];
    /* GOLD: */
    status_buf[9]  = 'G'; status_buf[10] = 'L'; status_buf[11] = 'D'; status_buf[12] = ':';
    u8_to_str(player_gold, tmp);
    for (i = 0; tmp[i]; i++) status_buf[13 + i] = tmp[i];
    /* DMG: */
    status_buf[19] = 'D'; status_buf[20] = 'M'; status_buf[21] = 'G'; status_buf[22] = ':';
    u8_to_str(player_dmg, tmp);
    for (i = 0; tmp[i]; i++) status_buf[23 + i] = tmp[i];

    plat_puts(0, STATUS_ROW, status_buf, COL_CYAN);
}

static void render_all(uint8_t px, uint8_t py) {
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
    draw_status();
}

static uint8_t step_onto(uint8_t *px, uint8_t *py, uint8_t nx, uint8_t ny) {
    int8_t ei;
    glyph_t target = map_get(nx, ny);
    if (map_is_solid(target)) return 0;

    ei = entity_at(nx, ny);
    if (ei >= 0) {
        entity_t *e = &entities[ei];
        if (e->g == G_ENEMY) {
            /* Bump attack. */
            e->hp -= (int8_t)player_dmg;
            if (e->hp <= 0) entity_kill((uint8_t)ei);
            return 0;  /* player doesn't move on attack */
        }
        /* Pickup. */
        switch (e->g) {
            case G_GOLD:   player_gold++;                       break;
            case G_POTION: if (player_hp < 250) player_hp += 3; break;
            case G_WEAPON: player_dmg++;                        break;
            default: break;
        }
        e->alive = 0;
    }
    *px = nx;
    *py = ny;
    return 1;
}

int main(void) {
    uint8_t key, nx, ny, px, py;
    uint8_t hit;

    plat_init();
    plat_seed_rand(0x1234);
    map_load(0);
    entity_init_from_map_spawns();

    px = map_player_x;
    py = map_player_y;
    render_all(px, py);

    for (;;) {
        key = plat_key_wait();
        nx = px; ny = py;
        switch (key) {
            case K_UP:    if (py > 0)         ny = py - 1; break;
            case K_DOWN:  if (py < map_h - 2) ny = py + 1; break;  /* -2 reserves status row */
            case K_LEFT:  if (px > 0)         nx = px - 1; break;
            case K_RIGHT: if (px < map_w - 1) nx = px + 1; break;
            case K_QUIT:  plat_shutdown(); return 0;
            default: continue;
        }
        if (nx != px || ny != py) {
            step_onto(&px, &py, nx, ny);
        }

        /* Enemy turn. */
        entity_ai_turn(px, py);

        /* Adjacent enemies damage player. */
        hit = entity_adjacent_damage(px, py);
        if (hit >= player_hp) {
            player_hp = 0;
            render_all(px, py);
            plat_puts(0, 0, "YOU DIED - press Q", COL_RED);
            for (;;) if (plat_key_wait() == K_QUIT) { plat_shutdown(); return 0; }
        } else {
            player_hp -= hit;
        }
        render_all(px, py);
    }
}
