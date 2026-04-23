#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Game state variables
extern bool run;
extern bool in_play;
extern bool obstruction;
extern bool screen_drawn;
extern bool draw_whole_screen;

// Player variables
extern unsigned char player_x;
extern unsigned char player_y;
extern unsigned char old_x;
extern unsigned char old_y;
extern unsigned char direction_x;
extern unsigned char direction_y;
extern unsigned char fx;
extern unsigned char fy;
extern unsigned char keys;
extern unsigned char idols;
extern unsigned char potion;
extern signed char health;
extern int magic;
extern unsigned int score;
extern bool sword;
extern unsigned char weapon;
extern unsigned char room;

// Utility variables
extern unsigned int this_row;
extern unsigned int this_col;
extern unsigned int timer;
extern unsigned int delay;
extern unsigned int counter;
extern unsigned char key;
extern unsigned char i;
extern unsigned char c;
extern char output[40];

// Enemy structure and variables
struct enemy {
    unsigned char tile;
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char old_x;
    unsigned char old_y;
    signed char health;
    unsigned char strength;
    unsigned char speed;
    unsigned char armour;
    // Fields for enhanced enemy behaviors
    unsigned char patrol_direction;
    signed char patrol_step;
};

extern struct enemy enemies[25];
extern unsigned int enemy_count;
extern unsigned int this_enemy;

// Map variables
extern unsigned char game_map[1000];

// Map dimensions (from maze.h)
#define MAP_WIDTH 40
#define MAP_HEIGHT 24
extern unsigned char info_row;

/* Reserve top 1 and bottom 2 lines for HUD */
#define HUD_TOP 1
#define HUD_BOTTOM 2
#define PLAYABLE_HEIGHT (MAP_HEIGHT - HUD_TOP - HUD_BOTTOM)

/* Define cell grid dimensions based on the playable area */
#define CELLS_X ((MAP_WIDTH - 1) / 4)
#define CELLS_Y ((PLAYABLE_HEIGHT - 1) / 4)

/* Maze dimensions after expansion from the cell grid */
#define MAZE_WIDTH  (CELLS_X * 4 + 1)
#define MAZE_HEIGHT (CELLS_Y * 4 + 1)

/* Passage dimensions in each cell */
#define CELL_WIDTH  3
#define CELL_HEIGHT 3

#endif /* GLOBALS_H */ 