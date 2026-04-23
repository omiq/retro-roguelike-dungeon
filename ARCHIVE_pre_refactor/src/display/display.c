#include "display.h"
#include <string.h>
#if defined (__CC65__)
#include <conio.h>
#else
#include "../include/notconio.h"
#endif


#ifndef __CC65__
void sleep_ms(int milliseconds)
{
    #ifdef WIN32
        Sleep(milliseconds);
    #elif _POSIX_C_SOURCE >= 199309L
        struct timespec ts;
        ts.tv_sec = milliseconds / 1000;
        ts.tv_nsec = (milliseconds % 1000) * 1000000;
        nanosleep(&ts, NULL);
    #else
        usleep(milliseconds * 1000);
    #endif
}
#endif
// Lookup table for translating characters to the new character set

char     translate_char(char c) {
    // Translate characters to the new character set using a switch statement
    #ifdef __C64__
    switch(c) {
        case 'h': return 'S';
        case 'i': return 'X';
        case '+': return 'B'; 
        case '*': return 'Q';
        case 'k': return '%';
        case 'g': return 169;
        case 'r': return 126;
        case '|': return '^';
        default: return c;
    }
    #else
    return c;
    #endif
}   



// Translate the character C to a selected character from the 
// new character set using a lookup table but only for c64 currently
void translate_cputcxy(int col, int row, char c) {

    // if c64 then translate the character
    #ifdef __C64__
        c = translate_char(c);
    #else
        // pass to cputcxy without changes
    #endif

    cputcxy(col, row, c);   

}


void output_message(void) {
    char blank[40];
    sprintf(blank, "%s", "                                      ");
    cputsxy(0, info_row, blank);
    cputsxy(1, info_row, output);

    // Clear the output message
    sprintf(output, "%s", blank);
    refresh();
}

void draw_screen(void) {
    // Draw whole screen
    int row, col;

    if (draw_whole_screen && screen_drawn == false) {
        for (row = 0; row < PLAYABLE_HEIGHT; row++) {
            for (col = 0; col < MAZE_WIDTH; col++) {
                translate_cputcxy(col, row, get_map(col, row));
            }
        }
        screen_drawn = true;
    } else {
        // Update the screen around the player with a set radius 
        update_fov(player_x, player_y, 2);
    }
}

void draw_momentary_object(unsigned int obj_old_x, unsigned int obj_old_y, 
                          unsigned int obj_x, unsigned int obj_y, 
                          unsigned int obj_tile, unsigned int delay) {
    // Replace tile
    translate_cputcxy(obj_old_x, obj_old_y, get_map(obj_old_x, obj_old_y));

    // Draw tile in new location
    translate_cputcxy(obj_x, obj_y, obj_tile); 
   
    // Delay
    timer = dumb_wait(delay);

    // Replace tile again
    translate_cputcxy(obj_x, obj_y, get_map(obj_x, obj_y));
}

void draw_move(bool replace) {
    // Delete the player character
    if (!replace) {
        set_map(old_x, old_y, '.');
    }

    // Draw new location
    translate_cputcxy(old_x, old_y, get_map(old_x, old_y));
    translate_cputcxy(player_x, player_y, 64); 
    set_map(player_x, player_y, 64);
}

void update_fov(int player_x, int player_y, int radius) {
    int dy, dx;
    for (dy = -radius; dy <= radius; dy++) {
        for (dx = -radius; dx <= radius; dx++) {
            int x = player_x + dx;
            int y = player_y + dy;

            // Ensure coordinates are within the map bounds
            if (x >= 0 && x < MAP_WIDTH-3 && y >= 0 && y < PLAYABLE_HEIGHT) {
                c = get_map(x, y);
                if (c == ' ') c = '.';
                translate_cputcxy(x, y, c);
            }
        }
    }
} 