#include "input.h"
#include "../logic/game_logic.h"
#include "../display/display.h"

#if defined (__CC65__)
#include <conio.h>
#include <cc65.h>
#else
#include "../include/notconio.h"
#include <time.h>
#endif

#if defined (__CC65__)
unsigned int dumb_wait(unsigned int delay) {
    for (timer = 0; timer < delay; timer++) {}
    return timer;
}

void echo(void) {
    return;
}

void refresh(void) {
    return;
}
#else
/* For Ncurses */
int kbhit(void) {
    int ch, r;

    // turn off getch() blocking and echo
    nodelay(stdscr, TRUE);
    noecho();

    // check for input
    ch = getch();
    if (ch == ERR)      // no input
        r = FALSE;
    else                // input
    {
        r = TRUE;
        ungetch(ch);
    }

    // restore block and echo
    echo();
    nodelay(stdscr, FALSE);
    return(r);
}

unsigned int dumb_wait(unsigned int delay) {
    if(delay<=1000) {
        sleep_ms(delay);
    } else {
        sleep_ms(200);
    }
    
    return delay;
}
#endif

unsigned char get_key(void) {
    // Check keys;
    switch (key = cgetc()) 
    { 
        case 'w':
            if (player_y > 0) player_y--; 
            break; 
        case 'a': 
            if (player_x > 0) player_x--; 
            break; 
        
        case 's': 
            if (player_y < MAP_HEIGHT) player_y++; 
            break; 
        case 'd': 
            if (player_x < MAP_WIDTH) player_x++; 
            break; 

        case 'A': 
        case 'o':
            if (sword == true) {
                draw_momentary_object(player_x-1, player_y, player_x-1, player_y, '-', 2000); 
                attack(10, player_x-1, player_y);
            }
            break;     

        case 'D': 
        case 'p':
            if (sword == true) {
                draw_momentary_object(player_x+1, player_y, player_x+1, player_y, '-', 2000); 
                attack(10, player_x+1, player_y);
            }
            break; 
        case 'f': 
            if (magic > 5) {
                magic -= 5;
                fx = player_x + direction_x;
                fy = player_y + direction_y;  

                c = get_map(fx, fy);
                while ((c == ' ' || c == '.') && magic > 0) {             
                    draw_momentary_object(fx, fy, fx, fy, '*', 200); 
                    magic -= 1;
                    fx = fx + direction_x;
                    fy = fy + direction_y;    
                    c = get_map(fx, fy);
                }

                attack(10, fx, fy);
            }
            break;            
        case 'Q':
        case 3:
            in_play = false;
            break;
        default:
            break; 
    }

    // debounce
    dumb_wait(100);

    return key;
} 