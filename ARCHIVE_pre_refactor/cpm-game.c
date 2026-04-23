/**

  copy to host OS with srctools/cpmr.sh -t c 0:game.c
  (copy while translating text file from drive c, user area 0, the file game.c

  When using desktop CPM emulator and Z88DK this works
  zcc +cpm -o GAME.COM cpm-game.c

**/
#include <stdio.h>

char x=10;
char y=10;
char c=32;
char oldx;
char oldy;

void gotoxy(char x,char y) {
     printf("\033[%d;%dH", (y), (x));
}

void main() {
     printf("\033[2J");
     printf("\033[?25l");
     gotoxy(0,0);
     printf("#######################################\n");

     for(y=1; y<20; y++) {
          printf("#                                     #\n");
     }
     y=10;
     gotoxy(0,20);
     printf("#######################################");
     gotoxy(x,y);
     while(c!=27) {
        oldx=x;
        oldy=y;          
        switch (c) 
        { 
            case 'w': 
                y--; 
                break; 
            case 'a': 
                x--; 
                break; 
            case 's': 
                y++; 
                break; 
            case 'd': 
                x++; 
                break; 
            default: 
                break; 
        }
          if(x<2 || x>38) x=oldx;
          if(y<2 || y>19) y=oldy;
          gotoxy(oldx,oldy);
          printf(" ");
          gotoxy(x,y);
          printf("@");
          gotoxy(0,0);
          printf("%02d,%02d",x,y);
          c=getch();
     }
     printf("\033[?25h");
}
