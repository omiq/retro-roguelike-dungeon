/*
 * entity.c — simple turn-based entity management.
 * Enemies step toward player each turn if not blocked; bump = attack.
 */
#include "entity.h"
#include "map.h"

entity_t     entities[ENTITY_MAX];
uint8_t      entity_count;
uint8_t      player_hp;
uint8_t      player_dmg;
uint8_t      player_gold;
move_event_t move_events[MOVE_EVENTS_MAX];
uint8_t      move_event_count;

void entity_init_from_map_spawns(void) {
    uint8_t i;
    entity_count = 0;
    for (i = 0; i < map_spawn_count && entity_count < ENTITY_MAX; i++) {
        entities[entity_count].x     = map_spawns[i].x;
        entities[entity_count].y     = map_spawns[i].y;
        entities[entity_count].g     = map_spawns[i].g;
        entities[entity_count].alive = 1;
        if (map_spawns[i].g == G_ENEMY) {
            entities[entity_count].hp  = 3;
            entities[entity_count].dmg = 1;
        } else {
            entities[entity_count].hp  = 0;
            entities[entity_count].dmg = 0;
        }
        entity_count++;
    }
    player_hp   = 10;
    player_dmg  = 2;
    player_gold = 0;
}

int8_t entity_at(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < entity_count; i++) {
        if (entities[i].alive && entities[i].x == x && entities[i].y == y)
            return (int8_t)i;
    }
    return -1;
}

void entity_kill(uint8_t idx) {
    if (idx >= entity_count) return;
    entities[idx].alive = 0;
    map_set(entities[idx].x, entities[idx].y, G_CORPSE);
}

/* Returns 1 if (dx,dy) step is a valid move (in bounds, non-solid, no other
 * entity there). (px,py) is the player position — enemies don't walk over
 * the player; that'd skip a combat turn. */
static uint8_t can_step(uint8_t from_i, int8_t dx, int8_t dy,
                        uint8_t px, uint8_t py) {
    int16_t nx = (int16_t)entities[from_i].x + dx;
    int16_t ny = (int16_t)entities[from_i].y + dy;
    uint8_t ux, uy;
    int8_t occ;
    if (nx < 0 || ny < 0) return 0;
    ux = (uint8_t)nx;
    uy = (uint8_t)ny;
    if (map_is_solid(map_get(ux, uy))) return 0;
    if (ux == px && uy == py) return 0;
    occ = entity_at(ux, uy);
    if (occ >= 0 && (uint8_t)occ != from_i) return 0;
    return 1;
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

void entity_ai_turn(uint8_t px, uint8_t py) {
    uint8_t i;
    int8_t  dx, dy, adx, ady;
    move_event_count = 0;
    for (i = 0; i < entity_count; i++) {
        if (!entities[i].alive) continue;
        if (entities[i].g != G_ENEMY) continue;
        dx = (px > entities[i].x) ? 1 : (px < entities[i].x) ? -1 : 0;
        dy = (py > entities[i].y) ? 1 : (py < entities[i].y) ? -1 : 0;
        adx = dx < 0 ? -dx : dx;
        ady = dy < 0 ? -dy : dy;
        /* Prefer larger axis first, fall back to other. */
        if (dx != 0 && adx >= ady) {
            if (can_step(i, dx, 0, px, py)) { record_move(i, dx, 0); continue; }
            if (dy != 0 && can_step(i, 0, dy, px, py)) { record_move(i, 0, dy); continue; }
        } else {
            if (dy != 0 && can_step(i, 0, dy, px, py)) { record_move(i, 0, dy); continue; }
            if (dx != 0 && can_step(i, dx, 0, px, py)) { record_move(i, dx, 0); continue; }
        }
    }
}

uint8_t entity_adjacent_damage(uint8_t px, uint8_t py) {
    uint8_t i, dmg = 0;
    int16_t dx, dy;
    for (i = 0; i < entity_count; i++) {
        if (!entities[i].alive || entities[i].g != G_ENEMY) continue;
        dx = (int16_t)entities[i].x - (int16_t)px;
        dy = (int16_t)entities[i].y - (int16_t)py;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx <= 1 && dy <= 1) dmg += entities[i].dmg;
    }
    return dmg;
}
