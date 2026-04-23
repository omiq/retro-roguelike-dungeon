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
#include "enemy_types.h"

/* Player position */
static uint8_t px, py;

/* HP lost per door bump without a key (matches archive dungeon_multi). */
#define DOOR_BUMP_HP_COST 10

static uint8_t colour_for_glyph(glyph_t g) {
    switch (g) {
        case G_WALL:   return COL_WHITE;
        case G_DOOR:
        case G_DOOR_AJAR:
                       return COL_YELLOW;
        case G_FLOOR:  return COL_BLUE;
        case G_ENEMY:  return COL_RED;
        case G_GOLD:   return COL_YELLOW;
        case G_POTION: return COL_MAGENTA;
        case G_WEAPON: return COL_CYAN;
        case G_STAIRS: return COL_GREEN;
        case G_CORPSE: return COL_RED;
        case G_HEALTH: return COL_RED;
        case G_MAGIC:  return COL_MAGENTA;
        case G_IDOL:   return COL_YELLOW;
        case G_BOLT:   return COL_YELLOW;
        case G_KEY:    return COL_YELLOW;
        default:       return COL_WHITE;
    }
}

/* Last movement direction — updated every time the player steps, used by
 * the fireball spell. Defaults to "right" so the first cast goes east.
 * Matches original dungeon_multi.c (direction_x, direction_y). */
static int8_t direction_x = 1;
static int8_t direction_y = 0;

/* Cast fireball: costs 5 magic to start, travels along (direction_x,
 * direction_y) animating G_BOLT through floor cells, consuming 1 magic
 * per step, stops on non-floor, attacks that cell for 10 damage.
 * Ported from dungeon_multi.c case 'f'. */
static void cast_fireball(uint8_t px, uint8_t py) {
    int16_t fx, fy;
    glyph_t c;
    if (player_magic <= 5) return;
    player_magic -= 5;
    fx = (int16_t)px + direction_x;
    fy = (int16_t)py + direction_y;
    while (fx >= 0 && fy >= 0 && fx < map_w && fy < map_h && player_magic > 0) {
        c = map_get((uint8_t)fx, (uint8_t)fy);
        /* Stop on any non-floor tile. Enemies occupy floor tiles (drawn on
         * top) so we still pass through entity cells — bolt hits whatever
         * is at the endpoint. */
        if (c != G_FLOOR) break;
        /* If an enemy sits here, stop and let attack() resolve. */
        if (entity_at((uint8_t)fx, (uint8_t)fy) >= 0) break;
        plat_putc((uint8_t)fx, (uint8_t)fy, G_BOLT, COL_YELLOW);
        plat_delay_ms(80);
        /* Restore underlying cell. */
        plat_putc((uint8_t)fx, (uint8_t)fy, c, colour_for_glyph(c));
        player_magic--;
        fx += direction_x;
        fy += direction_y;
    }
    /* Impact: attack whatever entity is there (if any). 10 damage matches
     * original attack(10, fx, fy). */
    if (fx >= 0 && fy >= 0 && fx < map_w && fy < map_h) {
        int8_t ei = entity_at((uint8_t)fx, (uint8_t)fy);
        if (ei >= 0 && entities[ei].alive && entities[ei].g == G_ENEMY) {
            uint8_t killed = entity_player_attack(px, py,
                                                  (uint8_t)fx, (uint8_t)fy, 10);
            if (killed) plat_putc((uint8_t)fx, (uint8_t)fy,
                                  G_CORPSE, colour_for_glyph(G_CORPSE));
        }
    }
}

/* Status bar drawn on the row immediately below the map. */
static char status_buf[41];
static uint8_t prev_hp, prev_gold, prev_dmg, prev_magic, prev_idols, prev_keys;

static void u8_to_str(uint8_t v, char *out) {
    uint8_t h = v / 100, t = (v / 10) % 10, o = v % 10, p = 0;
    if (h)      out[p++] = '0' + h;
    if (h || t) out[p++] = '0' + t;
    out[p++] = '0' + o;
    out[p] = '\0';
}

static void write_field(const char *label, uint8_t value, uint8_t col) {
    uint8_t i;
    char tmp[4];
    for (i = 0; label[i]; i++) status_buf[col + i] = label[i];
    u8_to_str(value, tmp);
    for (i = 0; tmp[i]; i++) status_buf[col + 3 + i] = tmp[i];
}

static void build_status(void) {
    uint8_t i;
    char tmp[4];
    for (i = 0; i < 40; i++) status_buf[i] = ' ';
    status_buf[40] = '\0';
    /* Layout: "HP:nn MP:nn $:nn K:n I:n/nn" in 40 cols. */
    write_field("HP:", player_hp,    0);
    write_field("MP:", player_magic, 7);
    status_buf[14] = '$'; status_buf[15] = ':';
    u8_to_str(player_gold, tmp);
    for (i = 0; tmp[i]; i++) status_buf[16 + i] = tmp[i];
    status_buf[21] = 'K'; status_buf[22] = ':';
    u8_to_str(player_keys, tmp);
    for (i = 0; tmp[i]; i++) status_buf[23 + i] = tmp[i];
    status_buf[27] = 'I'; status_buf[28] = ':';
    u8_to_str(player_idols, tmp);
    for (i = 0; tmp[i]; i++) status_buf[29 + i] = tmp[i];
    status_buf[29 + i] = '/';
    {
        char tmp2[4];
        uint8_t j;
        u8_to_str(idols_total, tmp2);
        for (j = 0; tmp2[j]; j++) status_buf[30 + i + j] = tmp2[j];
    }
}

static void redraw_status_if_changed(void) {
    if (player_hp    == prev_hp    &&
        player_gold  == prev_gold  &&
        player_dmg   == prev_dmg   &&
        player_magic == prev_magic &&
        player_idols == prev_idols &&
        player_keys  == prev_keys)
        return;
    build_status();
    plat_puts(0, map_h, status_buf, COL_CYAN);
    prev_hp    = player_hp;
    prev_gold  = player_gold;
    prev_dmg   = player_dmg;
    prev_magic = player_magic;
    prev_idols = player_idols;
    prev_keys  = player_keys;
}

/* Returns the colour to draw entity ei in (type colour for enemies, else default). */
static uint8_t colour_for_entity(int8_t ei) {
    int8_t t;
    if (ei < 0) return COL_WHITE;
    if (entities[ei].g == G_ENEMY) {
        t = entities[ei].type_idx;
        if (t >= 0 && t < ENEMY_TYPE_COUNT) return ENEMY_TYPES[t].colour;
    }
    return colour_for_glyph(entities[ei].g);
}

/* Redraw one cell from map + any live entity sitting on it. */
static void redraw_cell(uint8_t x, uint8_t y) {
    int8_t ei = entity_at(x, y);
    if (ei >= 0 && entities[ei].alive) {
        plat_putc(x, y, entities[ei].g, colour_for_entity(ei));
    } else {
        glyph_t g = map_get(x, y);
        plat_putc(x, y, g, colour_for_glyph(g));
    }
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
    /* Doors: key opens to floor in one step (archive + raylib).
     * Without a key, bumping costs 10 HP and progresses the tile:
     *   G_DOOR -> G_DOOR_AJAR (stay blocked this turn),
     *   G_DOOR_AJAR -> G_FLOOR (stay blocked this turn; walk in next turn).
     * At 0 HP you still pay the tile change; death is handled after the turn. */
    if (target == G_DOOR || target == G_DOOR_AJAR) {
        if (player_keys > 0) {
            player_keys--;
            map_set(nx, ny, G_FLOOR);
            plat_putc(nx, ny, G_FLOOR, colour_for_glyph(G_FLOOR));
        } else if (target == G_DOOR) {
            if (player_hp <= DOOR_BUMP_HP_COST)
                player_hp = 0;
            else
                player_hp -= DOOR_BUMP_HP_COST;
            map_set(nx, ny, G_DOOR_AJAR);
            plat_putc(nx, ny, G_DOOR_AJAR, colour_for_glyph(G_DOOR_AJAR));
            return 0;
        } else {
            if (player_hp <= DOOR_BUMP_HP_COST)
                player_hp = 0;
            else
                player_hp -= DOOR_BUMP_HP_COST;
            map_set(nx, ny, G_FLOOR);
            plat_putc(nx, ny, G_FLOOR, colour_for_glyph(G_FLOOR));
            return 0;
        }
    } else if (map_is_solid(target)) {
        return 0;
    }
    ei = entity_at(nx, ny);
    if (ei >= 0) {
        entity_t *e = &entities[ei];
        if (e->g == G_ENEMY) {
            /* Original mechanic: d20 vs armour+speed, miss = enemy free hit
             * if player adjacent. entity_player_attack() in entity.c. */
            uint8_t killed = entity_player_attack(*px, *py, nx, ny, player_dmg);
            if (killed) plat_putc(nx, ny, G_CORPSE, colour_for_glyph(G_CORPSE));
            return 0;  /* player stays put; bump consumed the turn */
        }
        /* Pickup. */
        switch (e->g) {
            case G_GOLD:   player_gold += 5;                     break;
            case G_HEALTH: if (player_hp < 245) player_hp += 10; break;
            case G_MAGIC:  if (player_magic < 250) player_magic++; break;
            case G_IDOL:   player_idols++;                       break;
            case G_WEAPON: player_dmg += 2;                      break;
            case G_POTION: if (player_hp < 250) player_hp += 5;  break;
            case G_KEY:    player_keys++;                        break;
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

/* Load one map and rebuild state derived from that map:
 * map tiles, spawned entities, and player spawn position. */
static void load_map_state(uint8_t map_id) {
    map_load(map_id);
    entity_init_from_map_spawns();
    px = map_player_x;
    py = map_player_y;
}

/* Program structure (high level):
 * 1. Boot platform + attract sequence (title/demo; richer on capable platforms)
 * 2. Playing: input → update → render; load next map when wired
 * 3. Game over / win overlays: wait for logical keys, then replay or quit
 * 4. Single shutdown on exit
 *
 * plat_key_wait() maps most letters to K_OTHER (not raw ASCII), so
 * menus use K_FIRE (space/return/z) and K_QUIT only. */

typedef enum {
    STATE_ATTRACT = 0, /* attract mode before first play / replay / win */
    STATE_PLAYING,     /* one turn at a time */
    STATE_GAME_OVER,   /* wait: replay or quit */
    STATE_WIN,         /* floor cleared; wait Q */
    STATE_QUIT
} game_state_t;

static void platform_boot(void) {
    plat_init();
    plat_seed_rand(0);
}

/* New run: reset player stats, load map 0, full redraw. Does not re-init
 * the platform (safe for “play again”). */
static void start_new_run(void) {
    direction_x = 1;
    direction_y = 0;
    player_hp    = 30;
    player_dmg   = 10;
    player_gold  = 0;
    player_magic = 0;
    player_idols = 0;
    player_keys  = 0;
    load_map_state(0);
    initial_render(px, py);
}

/* Placeholder attract: static text. On platforms that support it, this can
 * grow into a timed demo / scrolling banner without changing the state name. */
static void run_attract(void) {
    plat_cls();
    plat_puts(0, 0, "Dungeon  arrows/WASD  fire=Z/spc  Q=quit", COL_CYAN);
    plat_puts(0, 1, "Press FIRE (space/return/Z) to start", COL_WHITE);
}

/* One playing turn: block for input, move/attack, enemy phase, patch draw.
 * Returns next state (stay PLAYING unless death, win, or quit). */
static game_state_t run_playing_turn(void) {
    uint8_t key, nx, ny, moved, i;
    uint8_t old_px = px, old_py = py;

    key = plat_key_wait();
    if (key == K_QUIT)
        return STATE_QUIT;

    nx = px;
    ny = py;
    switch (key) {
        case K_UP:    if (py > 0)         ny = py - 1;
                      direction_x = 0; direction_y = -1; break;
        case K_DOWN:  if (py < map_h - 1) ny = py + 1;
                      direction_x = 0; direction_y =  1; break;
        case K_LEFT:  if (px > 0)         nx = px - 1;
                      direction_x = -1; direction_y = 0; break;
        case K_RIGHT: if (px < map_w - 1) nx = px + 1;
                      direction_x =  1; direction_y = 0; break;
        case K_FIRE:  cast_fireball(px, py); break;
        default:      return STATE_PLAYING;
    }

    moved = 0;
    if (nx != px || ny != py)
        moved = step_onto(&px, &py, nx, ny);
    if (moved) {
        redraw_cell(old_px, old_py);
        plat_putc(px, py, G_PLAYER, COL_YELLOW);
    }

    entity_ai_turn(px, py);
    for (i = 0; i < move_event_count; i++) {
        redraw_cell(move_events[i].ox, move_events[i].oy);
        plat_putc(move_events[i].nx, move_events[i].ny,
                  move_events[i].g, colour_for_glyph(move_events[i].g));
    }

    /* Damage from bumped player already applied in entity_ai_turn. */
    if (player_hp == 0) {
        redraw_status_if_changed();
        plat_puts(0, 0, "GAME OVER  FIRE=again  Q=quit", COL_RED);
        return STATE_GAME_OVER;
    }

    redraw_status_if_changed();

    if (idols_total > 0 && player_idols >= idols_total) {
        plat_puts(0, 0, "ALL IDOLS FOUND - press Q", COL_YELLOW);
        return STATE_WIN;
    }

    return STATE_PLAYING;
}

int main(void) {
    game_state_t state = STATE_ATTRACT;

    platform_boot();
    run_attract();

    while (1) {
        switch (state) {
        case STATE_ATTRACT: {
            uint8_t k = plat_key_wait();
            if (k == K_QUIT)
                state = STATE_QUIT;
            else if (k == K_FIRE) {
                start_new_run();
                state = STATE_PLAYING;
            }
            /* else stay in attract (frame already drawn) */
            break;
        }

        case STATE_PLAYING:
            state = run_playing_turn();
            break;

        case STATE_GAME_OVER: {
            uint8_t k = plat_key_wait();
            if (k == K_QUIT)
                state = STATE_QUIT;
            else if (k == K_FIRE) {
                start_new_run();
                state = STATE_PLAYING;
            }
            break;
        }

        case STATE_WIN:
            if (plat_key_wait() == K_QUIT)
                state = STATE_QUIT;
            break;

        case STATE_QUIT:
            plat_shutdown();
            return 0;
        }
    }
}
