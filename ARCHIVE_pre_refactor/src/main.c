#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "include/globals.h"
#include "logic/game_logic.h"
#include "display/display.h"
#include "input/input.h"
#include "screens/screens.h"

#ifdef __CC65__
#include <conio.h>
#else
#include "include/notconio.h"
#endif

int main() {
    /* Clear Screen */
    clrscr();

    /* Hide cursor */
    cursor(0);

    // Titles
    counter = title_screen();
    init();

    // Start running     
    run = true;
        
    // Should the program end?
    while (run) {
        // Initialize if not already running a game
        if (in_play != true) {
            init();
        }

        // Use current time as 
        // seed for random generator 
        srand(counter);

        // Set up the screen
        load_room();
        draw_screen();
        cputcxy(player_x, player_y, '@');

        // Game on!
        in_play = true;

        /* Loop until game ends */
        while (in_play) {
            game_loop();
        }

        // Try again or quit?
        run = game_over();
    }
    
    clrscr();
    sprintf(output, "goodbye!\n\n");
    output_message();
    cursor(1);

    #ifndef __CC65__
        endwin();
        resetterm();
    #endif
        
    return(0);
} 