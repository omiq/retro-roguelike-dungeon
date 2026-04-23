/*
 * entity.c — combat, pickups-as-entities, and enemy AI.
 *
 * Relationship to other game modules
 * -------------------------------------
 * - map.c     owns static geometry (walls, doors, floor). Living things and
 *             loose items live in entities[] after map_load + init here.
 * - main.c    owns the player cell (px, py), turn order, and all drawing.
 *             It calls entity_player_attack / entity_ai_turn each turn and
 *             consumes move_events[] to patch only changed screen cells.
 * - platform  supplies plat_rand and timing; this file never touches hardware.
 *
 * Data model
 * ----------
 * Every occupant of a map cell that is not the lone @ player is represented
 * as entity_t: position, glyph (G_FOE_* / G_GOLD / …), alive flag, hp/dmg.
 * Pickups use hp=0, dmg=0; enemies get stats from ENEMY_TYPES[type_idx].
 * entity_kill() syncs the map tile to G_CORPSE for dead enemies so the
 * floor glyph under the body stays consistent for redraw_cell().
 *
 * Original mechanics preserved (dungeon_multi.c port)
 * -----------------------------------------------------
 *   - Player bump-attack: d20 hit if rnum > armour+speed; on miss, if the
 *     player is orthogonally adjacent to the target, the enemy deals a free
 *     strength hit (same 4-dir + same-cell test as the original).
 *   - Enemy stepping onto the player: d20; >10 enemy hits by strength, else
 *     player counter-attacks for 5 fixed damage.
 *   - Rat: random NSEW each turn when awake. Goblin + kobold: greedy step
 *     toward player on both axes (can appear to cut corners).
 *
 * move_events[] protocol
 * -----------------------
 * entity_ai_turn clears move_event_count, then record_move() appends one
 * entry per successful enemy step (old cell, new cell, glyph for draw).
 * main.c redraws old from map+entities, then draws the enemy glyph on new.
 */
#include "entity.h"
#include "enemy_types.h"
#include "map.h"
#include "../platform/platform.h"

/* --- Global entity state and player inventory (see entity.h) --- */

entity_t     entities[ENTITY_MAX];
uint8_t      entity_count;
uint8_t      player_hp;
uint8_t      player_dmg;
uint8_t      player_gold;
uint8_t      player_magic;
uint8_t      player_idols;
uint8_t      idols_total;
uint8_t      player_keys;
move_event_t move_events[MOVE_EVENTS_MAX];
uint8_t      move_event_count;

/* Static species table: marker must match map.c template letters (G/R/T). */

const enemy_type_t ENEMY_TYPES[ENEMY_TYPE_COUNT] = {
    /* marker hp dmg spd arm colour          ai                 floor name */
    { 'G', 30, 5, 1, 10, COL_GREEN,   ENEMY_AI_GREEDY,   0, "goblin"   },
    { 'R', 15, 5, 2,  0, COL_CYAN,    ENEMY_AI_RANDOM4,  0, "rat"      },
    { 'T', 20, 4, 1,  5, COL_MAGENTA, ENEMY_AI_GREEDY,   0, "thug"     },
    { '&', 22, 4, 1,  6, COL_WHITE,   ENEMY_AI_GREEDY,   0, "skeleton" },
};

/* Lookup ENEMY_TYPES index from a template character; -1 if not an enemy. */

int8_t enemy_type_from_marker(char c) {
    uint8_t i;
    for (i = 0; i < ENEMY_TYPE_COUNT; i++)
        if (ENEMY_TYPES[i].marker == c) return (int8_t)i;
    return -1;
}

glyph_t enemy_foe_glyph_for_marker(char c) {
    switch (c) {
        case 'G': return G_FOE_GOBLIN;
        case 'R': return G_FOE_RAT;
        case 'T': return G_FOE_THUG;
        case '&': return G_FOE_SKELETON;
        default:  return G_FLOOR;
    }
}

/* Rebuild entities[] from map_game_objects[] after each map_load.
 * Player stats are not reset here — main.c start_new_run() owns run-wide
 * HP/gold/etc. This only rebuilds who sits on the map and counts idols for
 * the win condition (idols_total). */

void entity_init_from_map_game_objects(void) {
    uint8_t i;
    int8_t  t;
    entity_count = 0;
    for (i = 0; i < map_game_object_count && entity_count < ENTITY_MAX; i++) {
        entities[entity_count].x        = map_game_objects[i].x;
        entities[entity_count].y        = map_game_objects[i].y;
        entities[entity_count].g        = map_game_objects[i].g;
        entities[entity_count].alive    = 1;
        entities[entity_count].type_idx = map_game_objects[i].type_idx;
        if (enemy_glyph_is_foe(map_game_objects[i].g)) {
            t = map_game_objects[i].type_idx;
            if (t >= 0 && t < ENEMY_TYPE_COUNT) {
                entities[entity_count].hp  = (int8_t)ENEMY_TYPES[t].hp;
                entities[entity_count].dmg = ENEMY_TYPES[t].dmg;
            } else {
                entities[entity_count].hp  = 10;
                entities[entity_count].dmg = 2;
            }
        } else {
            entities[entity_count].hp  = 0;
            entities[entity_count].dmg = 0;
        }
        entity_count++;
    }

    idols_total  = 0;
    for (i = 0; i < entity_count; i++)
        if (entities[i].g == G_IDOL) idols_total++;
}

/* First live entity at (x,y), or -1. Used for combat, pickups, and drawing. */

int8_t entity_at(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < entity_count; i++)
        if (entities[i].alive && entities[i].x == x && entities[i].y == y)
            return (int8_t)i;
    return -1;
}

/* Mark dead and write G_CORPSE into map_tiles so redraw_cell shows the body
 * once the entity is no longer drawn as alive. */

void entity_kill(uint8_t idx) {
    if (idx >= entity_count) return;
    entities[idx].alive = 0;
    map_set(entities[idx].x, entities[idx].y, G_CORPSE);
}

/* --- Combat helpers (player bump + enemy bump into player) --- */

/* Uniform 1..20 inclusive, via plat_rand (platform-seeded PRNG). */

static uint8_t d20(void) {
    return (uint8_t)((plat_rand() % 20) + 1);
}

/* 4-dir or same-cell adjacency check, matching the original attack() fallback. */
static uint8_t player_adjacent(uint8_t px, uint8_t py, uint8_t ax, uint8_t ay) {
    if (px == ax && py == ay) return 1;
    if (px == ax && (py == ay + 1 || py + 1 == ay)) return 1;
    if (py == ay && (px == ax + 1 || px + 1 == ax)) return 1;
    return 0;
}

/* Player strikes enemy at (ax, ay) with weapon damage. Returns 1 if enemy
 * killed this turn. Ported logic: d20 > armour+speed = hit, else miss AND
 * if player is adjacent, enemy lands a free strength hit. */
uint8_t entity_player_attack(uint8_t px, uint8_t py, uint8_t ax, uint8_t ay,
                             uint8_t weapon) {
    int8_t  ei = entity_at(ax, ay);
    uint8_t rnum, armour_plus_speed;
    entity_t *e;
    int8_t  t;
    if (ei < 0) return 0;
    e = &entities[ei];
    if (!e->alive || !enemy_glyph_is_foe(e->g)) return 0;

    rnum = d20();
    t = e->type_idx;
    armour_plus_speed = (t >= 0 && t < ENEMY_TYPE_COUNT)
        ? (ENEMY_TYPES[t].armour + ENEMY_TYPES[t].speed)
        : 10;

    if (rnum > armour_plus_speed) {
        /* Hit! */
        e->hp -= (int8_t)weapon;
        if (e->hp <= 0) {
            entity_kill((uint8_t)ei);
            return 1;
        }
    } else {
        /* Miss. If player adjacent, enemy lands a free hit (original behaviour). */
        if (player_adjacent(px, py, ax, ay)) {
            if (player_hp <= e->dmg) player_hp = 0;
            else                     player_hp -= e->dmg;
        }
    }
    return 0;
}

/* Enemy attacks player when it tried to step onto player's cell. */
static void enemy_attack_player(uint8_t ei) {
    uint8_t rnum = d20();
    entity_t *e = &entities[ei];
    if (rnum > 10) {
        /* Enemy hits. */
        if (player_hp <= e->dmg) player_hp = 0;
        else                     player_hp -= e->dmg;
    } else {
        /* Miss → player counter-attack (original: enemy health -= 5). */
        e->hp -= 5;
        if (e->hp <= 0) entity_kill(ei);
    }
}

/* Check if cell (x,y) is walkable for an entity: in bounds, non-solid,
 * no other live entity, optionally the player cell returns the special
 * PLAYER_CELL sentinel so caller can convert to attack. */
#define WALK_FREE      1
#define WALK_BLOCKED   0
#define WALK_PLAYER    2
static uint8_t walkable(uint8_t from_i, int8_t dx, int8_t dy,
                        uint8_t px, uint8_t py) {
    int16_t nx = (int16_t)entities[from_i].x + dx;
    int16_t ny = (int16_t)entities[from_i].y + dy;
    uint8_t ux, uy;
    int8_t  occ;
    if (nx < 0 || ny < 0) return WALK_BLOCKED;
    ux = (uint8_t)nx; uy = (uint8_t)ny;
    if (map_is_solid(map_get(ux, uy))) return WALK_BLOCKED;
    if (ux == px && uy == py)          return WALK_PLAYER;
    occ = entity_at(ux, uy);
    if (occ >= 0 && (uint8_t)occ != from_i) return WALK_BLOCKED;
    return WALK_FREE;
}

static void record_move(uint8_t i, int8_t dx, int8_t dy) {
    if (move_event_count < MOVE_EVENTS_MAX) {
        move_events[move_event_count].ox = entities[i].x;
        move_events[move_event_count].oy = entities[i].y;
        move_events[move_event_count].nx = entities[i].x + dx;
        move_events[move_event_count].ny = entities[i].y + dy;
        move_events[move_event_count].g  = entities[i].g;
        move_event_count++;
    }
    entities[i].x += dx;
    entities[i].y += dy;
}

/* Squared-distance check vs AI_WAKE_RANGE (ported from raylib's
 * is_within_range). Keeps the arithmetic in uint16 to avoid float. */
static uint8_t within_wake_range(uint8_t px, uint8_t py,
                                 uint8_t ex, uint8_t ey) {
    int16_t dx = (int16_t)ex - (int16_t)px;
    int16_t dy = (int16_t)ey - (int16_t)py;
    uint16_t d2 = (uint16_t)(dx * dx + dy * dy);
    return d2 <= (AI_WAKE_RANGE * AI_WAKE_RANGE) ? 1 : 0;
}

/* One pass over all enemies: far-away enemies skip; others try one step.
 * Colliding with the player resolves to enemy_attack_player; free cells
 * enqueue move_events for the host to redraw without a full screen refresh. */

void entity_ai_turn(uint8_t px, uint8_t py) {
    uint8_t i;
    int8_t  dx, dy, t;
    uint8_t rnd, w;

    move_event_count = 0;
    for (i = 0; i < entity_count; i++) {
        if (!entities[i].alive || !enemy_glyph_is_foe(entities[i].g)) continue;
        /* Sleep if player far away. Rats stay put too — simpler + matches
         * raylib version, where only woken enemies move. */
        if (!within_wake_range(px, py, entities[i].x, entities[i].y)) continue;

        t = entities[i].type_idx;
        if (t < 0 || t >= ENEMY_TYPE_COUNT) t = 0;
        if (ENEMY_TYPES[t].ai == ENEMY_AI_RANDOM4) {
            /* Random: rand()%4+1 (1..4) -> 1=up 2=right 3=down 4=left. */
            rnd = (uint8_t)((plat_rand() % 4) + 1);
            dx = 0; dy = 0;
            if (rnd == 1) dy = -1;
            else if (rnd == 2) dx =  1;
            else if (rnd == 3) dy =  1;
            else if (rnd == 4) dx = -1;
        } else {
            /* ENEMY_AI_GREEDY: both axes toward player each turn. */
            dx = (px > entities[i].x) ?  1 : (px < entities[i].x) ? -1 : 0;
            dy = (py > entities[i].y) ?  1 : (py < entities[i].y) ? -1 : 0;
        }
        if (dx == 0 && dy == 0) continue;

        w = walkable(i, dx, dy, px, py);
        if (w == WALK_PLAYER) {
            enemy_attack_player(i);
        } else if (w == WALK_FREE) {
            record_move(i, dx, dy);
        } /* BLOCKED → stay put */
    }
}

/* Legacy hook from an earlier port phase: damage is applied when an enemy
 * tries to enter the player tile (enemy_attack_player). Kept so callers or
 * headers that still declare it do not need a second ABI churn. */
uint8_t entity_adjacent_damage(uint8_t px, uint8_t py) {
    (void)px; (void)py;
    return 0;
}
