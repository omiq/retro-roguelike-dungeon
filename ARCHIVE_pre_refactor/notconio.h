/* 
    Duplicate or similar to CONIO functions
*/


#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>

/* Turn on/off the cursor */
void cursor(char onoff) {
    curs_set(onoff);
    refresh();
}
 
/* Clear screen and set up Curses */
void clrscr() 
{
 
    /* Initialise the screen */
    initscr();
    erase();
    noecho();
    raw();
    move(0, 0);
 
    /* Cursor off */
    curs_set(0);
    refresh();
 
}
 
/* Move cursor to coordinate */
void gotoxy(unsigned int x, unsigned int y)
{
    move(y,x);
    refresh();
}

/* Put char at coordinate */
void cputcxy(unsigned int x, unsigned int y, char outString)
{
    mvaddch(y,x,outString);
    refresh();
}

/* Put string at coordinate */
void cputsxy(unsigned int x, unsigned int y, char outString[40])
{
    mvprintw(y, x, "%s", outString);
}
 

/* Get keypress */
int cgetc() {
    return getch();
}

/* END OF CONIO */