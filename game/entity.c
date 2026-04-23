/*
 * entity.c — combat + movement ported from
 * ARCHIVE_pre_refactor/dungeon_multi.c (attack, enemy_attack, move_enemies).
 *
 * Original mechanics preserved:
 *   - player bump-attack rolls d20, hits when rnum > armour+speed
 *   - on miss, if player adjacent to enemy, enemy lands free strength hit
 *   - enemy_attack rolls d20, >10 → enemy damages player by strength,
 *     else player counter-attacks for 5
 *   - rat moves random (1..4 → NSEW)
 *   - goblin + kobold chase player both axes at once (diagonal-capable)
 *
 * One honest simplification: original measured player-adjacent via
 *   (x == ax && y == ay) || ... 4-dir checks. Ported verbatim.
 */
#include "entity.h"
#include "enemy_types.h"
#include "map.h"
#include "../platform/platform.h"

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

const enemy_type_t ENEMY_TYPES[ENEMY_TYPE_COUNT] = {
    /* marker, hp, dmg(str), spd, arm, colour,       name */
    {  'G',    30,   5,        1,  10, COL_GREEN,    "goblin" },
    {  'R',    15,   5,        2,   0, COL_CYAN,     "rat"    },
    {  'K',    20,   4,        1,   5, COL_MAGENTA,  "kobold" },
};

int8_t enemy_type_from_marker(char c) {
    uint8_t i;
    for (i = 0; i < ENEMY_TYPE_COUNT; i++)
        if (ENEMY_TYPES[i].marker == c) return (int8_t)i;
    return -1;
}

void entity_init_from_map_spawns(void) {
    uint8_t i;
    int8_t  t;
    entity_count = 0;
    for (i = 0; i < map_spawn_count && entity_count < ENTITY_MAX; i++) {
        entities[entity_count].x        = map_spawns[i].x;
        entities[entity_count].y        = map_spawns[i].y;
        entities[entity_count].g        = map_spawns[i].g;
        entities[entity_count].alive    = 1;
        entities[entity_count].type_idx = map_spawns[i].type_idx;
        if (map_spawns[i].g == G_ENEMY) {
            t = map_spawns[i].type_idx;
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

int8_t entity_at(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < entity_count; i++)
        if (entities[i].alive && entities[i].x == x && entities[i].y == y)
            return (int8_t)i;
    return -1;
}

void entity_kill(uint8_t idx) {
    if (idx >= entity_count) return;
    entities[idx].alive = 0;
    map_set(entities[idx].x, entities[idx].y, G_CORPSE);
}

/* d20 roll: 1..20. Ported from dungeon_multi.c (rand % 20 + 1). */
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
    if (!e->alive || e->g != G_ENEMY) return 0;

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

void entity_ai_turn(uint8_t px, uint8_t py) {
    uint8_t i;
    int8_t  dx, dy;
    uint8_t rnd, w;

    move_event_count = 0;
    for (i = 0; i < entity_count; i++) {
        if (!entities[i].alive || entities[i].g != G_ENEMY) continue;
        /* Sleep if player far away. Rats stay put too — simpler + matches
         * raylib version, where only woken enemies move. */
        if (!within_wake_range(px, py, entities[i].x, entities[i].y)) continue;

        if (entities[i].type_idx == ENEMY_TYPE_RAT) {
            /* Random: rand()%4+1 (1..4) -> 1=up 2=right 3=down 4=left. */
            rnd = (uint8_t)((plat_rand() % 4) + 1);
            dx = 0; dy = 0;
            if (rnd == 1) dy = -1;
            else if (rnd == 2) dx =  1;
            else if (rnd == 3) dy =  1;
            else if (rnd == 4) dx = -1;
        } else {
            /* Goblin + kobold: both axes toward player each turn. */
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

/* Kept for API compatibility but no longer called: original mechanics
 * handle damage via enemy_attack_player when enemies step into player. */
uint8_t entity_adjacent_damage(uint8_t px, uint8_t py) {
    (void)px; (void)py;
    return 0;
}
