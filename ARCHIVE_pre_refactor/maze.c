#include "maze.h"
#include <conio.h>
#include <cc65.h>

int main(void) {
    int i;
    srand((unsigned)time(NULL));
    clrscr();

    carveMaze();
    placePlayer();

    /* Print top HUD line */
    printf("HUD: Score 0\n");

    /* Print the playable area */
    for (i = 0; i < 22; i++) {
        printf("%s\n", map[i]);
    }

    /* Print bottom HUD lines */
    printf("Status: OK\n");
    printf("Commands: WASD to move");
    i=getchar();
    return 0;
}
