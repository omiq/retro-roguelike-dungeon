#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "../include/globals.h"

// Game initialization and loop
void init(void);
void game_loop(void);

// Map functions
unsigned char get_map(char x, char y);
void set_map(char x, char y, int tile);
void load_room(void);

// Enemy functions
unsigned int which_enemy(unsigned int ex, unsigned int ey);
void attack(unsigned int weapon, unsigned int ax, unsigned int ay);
void enemy_attack(unsigned int this_enemy);
bool is_within_range(int player_x, int player_y, int enemy_x, int enemy_y, int range);
void move_enemies(void);

#endif /* GAME_LOGIC_H */ 