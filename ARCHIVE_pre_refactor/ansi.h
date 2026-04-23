
    #include <stdio.h>
    
    
    void gotoxy(unsigned char x, unsigned char y) {
        printf("\e[%d;%dH", y, x); //	Move the cursor to x, y coordinate

    }

    void cursor(bool on) {
        if(!on) { printf("\e[?25l"); } else //	Hide the cursor
        { printf("\e[?25h"); } //	Show the cursor.

    }

    /* Put char at coordinate */
    void cputcxy(unsigned int x, unsigned int y, unsigned char outString)
    {
        gotoxy(x,y);
        putchar(outString);
    }

    void printw(char* fmt, char* str) {
        printf(fmt, str);
    }

    void mvprintw(unsigned char my, unsigned char mx, char* str) {
        gotoxy(mx,my);
        printw("%s", str);
    }


    void clrscr() {

        printf("\e[2J"); //	Clears the window/terminal
        gotoxy(0,0);
    }


    void refresh() {
        gotoxy(0,0);
        return;
    }

    /* Get key input     */
    /* requires stdlib.h */
    
    unsigned char getch()
    {
        char ch;
        system("stty raw -echo");
        ch=getchar();
        system("stty -raw echo");
        return ch;
    }
    

    unsigned char cgetc() {
        return getch();
    }
    


