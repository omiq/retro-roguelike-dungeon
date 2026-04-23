#ifndef DISPLAY_H
#define DISPLAY_H

#include "../include/globals.h"
#include "../logic/game_logic.h"
#include "../input/input.h"

void sleep_ms(int milliseconds);


// Display functions
void output_message(void);
void draw_screen(void);
void draw_momentary_object(unsigned int obj_old_x, unsigned int obj_old_y, 
                          unsigned int obj_x, unsigned int obj_y, 
                          unsigned int obj_tile, unsigned int delay);
void draw_move(bool replace);
void update_fov(int player_x, int player_y, int radius);

void translate_cputcxy(int col, int row, char c);


#endif /* DISPLAY_H */ 