/* 
    Duplicate or similar to CONIO functions
*/

#ifndef NOTCONIO_H
#define NOTCONIO_H

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>

/* Function declarations */
void cursor(char onoff);
void clrscr(void);
void gotoxy(unsigned int x, unsigned int y);
void cputcxy(unsigned int x, unsigned int y, char outString);
void cputsxy(unsigned int x, unsigned int y, char outString[40]);
int cgetc(void);
void revers(char onoff);
#endif /* NOTCONIO_H */ 