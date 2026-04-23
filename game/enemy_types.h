/*
 * enemy_types.h — static enemy species table.
 * Map template markers K/G/R/H select an index into ENEMY_TYPES.
 * Ported from ARCHIVE_pre_refactor/dungeon_multi.c load_room():
 *   goblin hp=30 str=5 spd=1 arm=10  (char 38 in original)
 *   rat    hp=15 str=5 spd=2 arm=0   (char 94 in original)
 * K (kobold) and H (horror) existed in map templates but weren't
 * given stats in the original; restored here with new values.
 */
#ifndef ENEMY_TYPES_H
#define ENEMY_TYPES_H

#include <stdint.h>

#define ENEMY_TYPE_GOBLIN 0
#define ENEMY_TYPE_RAT    1
#define ENEMY_TYPE_KOBOLD 2
#define ENEMY_TYPE_COUNT  3

typedef struct {
    char    marker;      /* ASCII marker in room template */
    uint8_t hp;
    uint8_t dmg;
    uint8_t speed;       /* reserved for future AI weight */
    uint8_t armour;      /* reserved for future hit-chance */
    uint8_t colour;      /* COL_* for display */
    char    name[8];
} enemy_type_t;

/* Kept as extern+definition-in-entity.c so adapters don't each instantiate a copy. */
extern const enemy_type_t ENEMY_TYPES[ENEMY_TYPE_COUNT];

int8_t enemy_type_from_marker(char c);   /* -1 if not an enemy marker */

#endif
