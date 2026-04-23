/*
 * entity.h — live entities (enemies, items on floor).
 * Map game objects get converted into entity_t records on init so each can
 * move (enemies) and be removed (pickups, kills).
 */
#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include <stdint.h>
#include "glyphs.h"

#define ENTITY_MAX 32

typedef struct {
    uint8_t x, y;
    glyph_t g;
    uint8_t alive;      /* 0 = free slot, 1 = live */
    int8_t  hp;
    uint8_t dmg;
    int8_t  type_idx;   /* ENEMY_TYPE_* for enemies, -1 otherwise */
} entity_t;

extern entity_t entities[ENTITY_MAX];
extern uint8_t  entity_count;

/* Player stats — kept here so entity_step_toward_player can read them. */
extern uint8_t player_hp;
extern uint8_t player_dmg;
extern uint8_t player_gold;
extern uint8_t player_magic;
extern uint8_t player_idols;
extern uint8_t idols_total;    /* set at map_load from idol object count */
extern uint8_t player_keys;    /* unlock doors */

/* Wake range for AI — enemies within this tile radius chase player.
 * Ported from raylib version's is_within_range(player, enemy, 6). */
#define AI_WAKE_RANGE 6

/* Move event buffer — entity_ai_turn fills this so render can redraw
 * only the cells that changed instead of the whole screen. */
#define MOVE_EVENTS_MAX ENTITY_MAX
typedef struct {
    uint8_t ox, oy;   /* cell the entity left */
    uint8_t nx, ny;   /* cell the entity arrived at */
    glyph_t g;        /* entity glyph (for drawing new cell) */
} move_event_t;

extern move_event_t move_events[MOVE_EVENTS_MAX];
extern uint8_t      move_event_count;

void     entity_init_from_map_game_objects(void);
int8_t   entity_at(uint8_t x, uint8_t y);   /* returns index or -1 */
void     entity_kill(uint8_t idx);           /* corpse on map, entity gone */
void     entity_ai_turn(uint8_t px, uint8_t py);
uint8_t  entity_adjacent_damage(uint8_t px, uint8_t py);

/* Player bumps enemy at (ax, ay) with weapon damage. Returns 1 if enemy
 * was killed this turn. Ported d20 + armour+speed hit check. */
uint8_t  entity_player_attack(uint8_t px, uint8_t py,
                              uint8_t ax, uint8_t ay, uint8_t weapon);

#endif
