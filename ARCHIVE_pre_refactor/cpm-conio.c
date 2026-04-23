/*
	Requires console compatiblity 
	zcc +cpm -o cpm-conio.com -subtype=z80pack -create-app cpm-conio.c --generic-console -pragma-define:CONSOLE_COLUMNS=80 -pragma-define:CONSOLE_ROWS=24

	vio zcc +cpm -o cpm-conio.com -subtype=z80pack -create-app cpm-conio.c --generic-console -pragma-define:CONSOLE_COLUMNS=80 -pragma-define:CONSOLE_ROWS=24 --vio

*/
#include <stdio.h>
#include <conio.h>
#include <ctype.h>
// Global key variable
int key;
char x,y=10;
int main()
{

    /* Cursor off */ 
    printf("\e[1;1H\e[?25l");

    /* Clear Screen */
    clrscr();


    /* Loop until Q is pressed */
    while ((key  = toupper(cgetc())) != 'Q')
    {
        // Delete the character
        gotoxy(x,y);
        putch('.');
        // keys;
        switch (key) 
        { 
            case 'W': 
            case 'w': 
                y--; 
                break; 
            case 'A':
            case 'a':  
                x--; 
                break; 
            case 'S': 
            case 's': 
                y++; 
                break; 
            case 'D':
            case 'd': 
                x++; 
                break; 
            default: 
                break; 
        }
        gotoxy(x,y);
        putch('@');
    
    }
    
    // Cursor on
    printf("\e[?25h");
    return(0);
}

