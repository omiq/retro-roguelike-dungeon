/*
 * enemy_types.h — enemy species table + foe-glyph helpers.
 *
 * Map markers (see map.c) are one ASCII char per species ('&' = skeleton).
 * Lowercase 'k' is reserved for keys — thug uses 'T' (not 'K'). To add a new foe: bump
 * ENEMY_TYPE_COUNT, append ENEMY_TYPES[], add G_FOE_* in glyphs.h, extend
 * enemy_foe_glyph_for_marker() + enemy_glyph_is_foe(), and append one char
 * per new id in every platform glyph_native[].
 *
 * Fields like ai / speed / armour are for combat + future patrol/loot/difficulty
 * tables without reshaping entity_t.
 */
#ifndef ENEMY_TYPES_H
#define ENEMY_TYPES_H

#include <stdint.h>
#include "glyphs.h"

#define ENEMY_TYPE_GOBLIN 0
#define ENEMY_TYPE_RAT    1
#define ENEMY_TYPE_THUG   2
#define ENEMY_TYPE_SKELETON 3
#define ENEMY_TYPE_COUNT  4

/* Patrol / step intent (entity_ai_turn switches on this). */
#define ENEMY_AI_GREEDY   0   /* move on both axes toward player each turn */
#define ENEMY_AI_RANDOM4  1   /* rand NSEW when awake */
#define ENEMY_AI_PATROLNSEW   2   /* patrol the room all directions*/
#define ENEMY_AI_PATROLNS   3   /* patrol the room north and south */ 
#define ENEMY_AI_PATROLEW   4   /* patrol the room east and west */

typedef struct {
    char    marker;      /* ASCII marker in room template (unique) */
    uint8_t hp;
    uint8_t dmg;
    uint8_t speed;      /* used in hit chance (armour+speed) */
    uint8_t armour;
    uint8_t colour;      /* COL_* for display */
    uint8_t ai;          /* ENEMY_AI_* */
    uint8_t min_floor;   /* 0 = any; future: first dungeon depth this may spawn */
    char    name[12];
} enemy_type_t;

extern const enemy_type_t ENEMY_TYPES[ENEMY_TYPE_COUNT];

int8_t  enemy_type_from_marker(char c);
glyph_t enemy_foe_glyph_for_marker(char c);

/* True if g is a foe logical glyph (see G_FOE_* in glyphs.h). */
#define enemy_glyph_is_foe(g) \
    ((g) == G_FOE_GOBLIN || (g) == G_FOE_RAT || (g) == G_FOE_THUG || (g) == G_FOE_SKELETON)

#endif
