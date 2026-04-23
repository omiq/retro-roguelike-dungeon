/*
 * entity.h — live entities (enemies, items on floor).
 * Map spawns get converted into entity_t records on init so each can
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
    uint8_t alive;   /* 0 = free slot, 1 = live */
    int8_t  hp;
    uint8_t dmg;
} entity_t;

extern entity_t entities[ENTITY_MAX];
extern uint8_t  entity_count;

/* Player stats — kept here so entity_step_toward_player can read them. */
extern uint8_t player_hp;
extern uint8_t player_dmg;
extern uint8_t player_gold;

void     entity_init_from_map_spawns(void);
int8_t   entity_at(uint8_t x, uint8_t y);   /* returns index or -1 */
void     entity_kill(uint8_t idx);           /* corpse on map, entity gone */
void     entity_ai_turn(uint8_t px, uint8_t py);
uint8_t  entity_adjacent_damage(uint8_t px, uint8_t py);

#endif
