/*

This version will form the basis using modern ascii
conversions will then use header files and definitions

 cc dungeon.c -o ~/dungeon -lncurses

 install ncurses: sudo apt-get install libncurses-dev

 
    cl65 -t pet -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o dungeonPET.prg dungeon.c
    cl65 -t c64 -v -o dungeon64.prg dungeon.c && x64sc dungeon64.prg > /dev/null
 
*/



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>


// Global variables due to CC65 not liking local
bool run=true;
bool in_play=false;
bool obstruction=false;
bool screen_drawn=false;
unsigned int this_row;
unsigned int this_col;
unsigned int timer,delay,counter;
unsigned char key,i,c, weapon;
unsigned char player_x=19;
unsigned char player_y=8;
unsigned char old_x, old_y, direction_x, direction_y, fx, fy;
unsigned char room;
bool draw_whole_screen = false;
//unsigned char buffer [sizeof(int)*8+1];



#if defined (__CC65__)
#include<cc65.h>
#include <conio.h>

#if defined __CBM__
    #include <cbm.h>
    #include <peekpoke.h>

    #define VIC_BASE_RAM			(0x8000)
    #define SCREEN_RAM				((char*)VIC_BASE_RAM+0x0400)
    #define CHARMAP_RAM				((char*)VIC_BASE_RAM+0x0800)
    #define SCREEN_WIDTH			40
    #define SCREEN_HEIGHT			25
    #define COLOR_OFFSET			(int)(COLOUR_RAM - SCREEN_RAM)
    #define CHARS_RAM			    0x80
    #define CHAR_SPACE				0x20
    #define RAM_BLOCK				(CHARS_RAM + CHAR_SPACE)
    #define CHARMAP_DEST			(RAM_BLOCK + 1)


unsigned int dumb_wait(unsigned int delay) {
    for(timer=0; timer<delay; timer++) {}
    return timer;
}

void echo(void) {
    return;
}

void refresh(void) {
    return;
}


#endif
#else
    #include "notconio.h"
    #include <time.h>


    /* For Ncurses */
    int kbhit(void)    
    {
        int ch, r;

        // turn off getch() blocking and echo
        nodelay(stdscr, TRUE);
        noecho();

        // check for input
        ch = getch();
        if( ch == ERR)      // no input
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
   

    // Storing start time
    clock_t start_time = clock();
    int counter = 0;
    // looping till required time is not achieved
    while (clock() < start_time + delay*250)
       
        counter++;
        //printf("%d",counter);
        ;
    return delay;
}

#endif



// Player 
unsigned char keys,idols,potion=0;
signed char health;
int magic;
unsigned int score;
bool sword = false;


// Enemy
unsigned int enemy_count=0;
struct enemy {
    unsigned char tile;
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char old_x;
    unsigned char old_y;
    signed char health;
    unsigned char strength;
    unsigned char speed;
    unsigned char armour;
};
unsigned int this_enemy = 0;

struct enemy enemies[25];


#include "maze.h"
char output [MAP_WIDTH];


    void output_message() {
        char blank[40];
        sprintf(blank, "%s", "                                      ");
        cputsxy(0,info_row,blank);
        cputsxy(1,info_row,output);
        sprintf(output, "%s",blank);
        refresh();
    }



// (re)initialize game
void init(void) {
            keys=0;
            health=100;
            score=0;
            keys=0;
            room=2;
            potion=0;
            magic = 0;
            enemy_count=0;
            sword=false;
            weapon = 1;
            idols=0;
            draw_whole_screen = false;
            screen_drawn=false;
}



unsigned char game_map[1000];

unsigned char get_map(char x, char y) {
    
    unsigned char c;

    c = game_map[MAP_WIDTH*y+x];
    if(c==0) {return '.';}
    return c;
  

}


void load_room() {
    int pos=0;
    int playable_offsetY, offsetX, offsetY;
    draw_whole_screen=false;
    playable_offsetY = HUD_TOP;
    offsetX = (MAP_WIDTH - MAZE_WIDTH) / 2;
    offsetY = playable_offsetY + ((PLAYABLE_HEIGHT - MAZE_HEIGHT) / 2);

    clrscr();
    
    sprintf(output, "loading room %d",room);
    output_message();

    srand((unsigned)time(NULL));
    carveMaze();
    placePlayer();

    // 1 Gobbo less than per room level
    for(i=0; i<room-1; i++) placeObject('g');
    

    // 1 piece of idol per room level
    for(i=0; i<room; i++) placeObject('i');
    
    // Increase rats per room level   
    for(i=0; i<room+1; i++) placeObject('r');

    placeObject('*');
    placeObject('|');
    placeObject('$');
    placeObject('h');
    placeObject('k');
    placeObject('s'); // spell
    placeHDoor();
    placeVDoor();

    /* Copy the generated maze into the full map.
       First fill the entire map with walls. */
    for (this_row = 0; this_row < PLAYABLE_HEIGHT; this_row++) {
        for (this_col = 0; this_col < MAP_WIDTH; this_col++) {
            game_map[pos] = '#';   
            pos++;  
        }
    }

    pos = 0;
    for (this_row = 0; this_row < PLAYABLE_HEIGHT; this_row++) {  
        for(this_col = 0; this_col < MAP_WIDTH; this_col++) { 
        
        c=map[this_row][this_col];

        // Player x and y
        if(c=='@') {
            player_y=this_row;
            player_x=this_col;
        }

        // Goblin
        if(c=='g') {

            // Increment for next enemy (Enemy 0 is counted as no enemy)
            enemy_count+=1;

            // Create the enemy in the list
            enemies[enemy_count].tile = c;
            enemies[enemy_count].room = room;
            enemies[enemy_count].x = this_col;
            enemies[enemy_count].y = this_row;
            enemies[enemy_count].old_x = enemies[enemy_count].x;
            enemies[enemy_count].old_y = enemies[enemy_count].y;
            enemies[enemy_count].health = 30;
            enemies[enemy_count].strength = 5;
            enemies[enemy_count].speed = 1;
            enemies[enemy_count].armour = 10;

        }  

        // Rat
        else if (c=='r') {

            // Increment for next enemy (Enemy 0 is counted as no enemy)
            enemy_count+=1;

            // Create the enemy in the list
            enemies[enemy_count].tile = c;
            enemies[enemy_count].room = room;
            enemies[enemy_count].x = this_col;
            enemies[enemy_count].y = this_row;
            enemies[enemy_count].old_x = enemies[enemy_count].x;
            enemies[enemy_count].old_y = enemies[enemy_count].y;
            enemies[enemy_count].health = 15;
            enemies[enemy_count].strength = 5;
            enemies[enemy_count].speed = 2;
            enemies[enemy_count].armour = 0;

        }  
        //if(c=='.') c=' ';
        game_map[pos] = c;   
        pos++;  
        }
    }
    
    /*
    sprintf(output, "\n\nenemies %3d\n", enemy_count);
    for(i=1; i<enemy_count+1;i++)
    {
        sprintf(output, "%s %3d\n",enemies[i].tile,i);
    }

    key = cgetc();      
    */
}

void set_map(char x, char y, int tile) {
    
    // Set the part of the array to the given tile
    game_map[MAP_WIDTH*y+x]=tile;
    
}





// Returns the enemy for a given x,y coord
unsigned int which_enemy(unsigned int ex,unsigned int ey) {

    if(get_map(ex,ey)==' '||get_map(ex,ey)=='.') return 0;

    // Enemies starts at 1, 0 = no enemy
    for(i=1;i<enemy_count+1;i++)
    {
        if(enemies[i].x == ex && enemies[i].y == ey && enemies[i].health >= 1) return i;
       
    }

    // No enemies
    return 0;

}


void attack(unsigned int weapon, unsigned int ax, unsigned int ay)
{
    int rnum = 0;
    this_enemy = 0;
    this_enemy = which_enemy(ax,ay);
    if(this_enemy == 0) {
        return;
    } 

    rnum = (rand() % (20 - 1 + 1)) + 1; 

    if(rnum > enemies[this_enemy].armour+enemies[this_enemy].speed) {

        // Damage!
        enemies[this_enemy].health-=weapon;
        if(enemies[this_enemy].health>0) 
        {
            
            
            sprintf(output, "hit!! enemy health: %3d    ", enemies[this_enemy].health);
            output_message();
            timer=dumb_wait(1000);
        }


        
    } else {
        

        sprintf(output, "miss! enemy health: %3d ", enemies[this_enemy].health);
        output_message();
        if((player_x == ax && player_y == ay)||(player_x == ax && (player_y == ay + 1 || player_y == ay - 1)) || (player_y == ay && (player_x == ax + 1 || player_x == ax - 1))) 
        {
            health -= enemies[this_enemy].strength;
        }
        timer=dumb_wait(1000);
    }

    if(enemies[this_enemy].health < 1 ) {

        // Success!
        
        sprintf(output, "enemy defeated!");
        output_message();
        
        // Draw tile in new location
        cputcxy(ax,ay,'.'); 
        set_map(ax,ay,'.');
        enemies[this_enemy].tile = '.';

        // Up the score
        score+=10;
        timer=dumb_wait(1000);
        
    }

}

void enemy_attack(unsigned int this_enemy)
{
    int rnum = 0;
    rnum = (rand() % (20 - 1 + 1)) + 1; 

    if(rnum > 10) {

        // Damage!
        if(health < 1 || (health-enemies[this_enemy].strength) < 1) 
        {
            health = 0;

        } else {
            health-=enemies[this_enemy].strength;
        }    
        
        sprintf(output, "ouch! health: %3d", health);
        output_message();
        timer=dumb_wait(1000);

        
    } else {
            enemies[this_enemy].health -= 5;
            if(enemies[this_enemy].health<1) {
               enemies[this_enemy].health=0;
                // Draw tile in new location
                cputcxy(enemies[this_enemy].x,enemies[this_enemy].y,'.'); 
                set_map(enemies[this_enemy].x,enemies[this_enemy].y,'.');
                enemies[this_enemy].tile = '.';
                
                sprintf(output, "enemy defeated!");
                output_message();
            }else {
                
                sprintf(output, "block health: %3d", health);}
                output_message();
        }
        
        timer=dumb_wait(1000);
    

    if(health < 1) {

        // Fail!
        
        sprintf(output, "enemy defeated you!");
        output_message();
        health = 0;
        timer=dumb_wait(1000);
    }

}

// How close is the player?
bool is_within_range(int player_x, int player_y, int enemy_x, int enemy_y, int range) {
    int dx = enemy_x - player_x;
    int dy = enemy_y - player_y;
    return (dx * dx + dy * dy) <= (range * range);
}

// Move the enemies for a given room
void move_enemies() {

    unsigned char rnd;

    // Enemies starts at 1, 0 = no enemy
    for(i=1;i<enemy_count+1;i++)
    {
        if(enemies[i].room == room && enemies[i].health>0){
            
            enemies[i].old_x = enemies[i].x;
            enemies[i].old_y = enemies[i].y; 
            
            // Rat is random
            if(enemies[i].tile == 'r' && is_within_range(player_x, player_y, enemies[i].x, enemies[i].y, 6)) {
                rnd = (rand() % (4)) + 1; 
                if(rnd == 4) enemies[i].x-=1;
                if(rnd == 2) enemies[i].x+=1;
                if(rnd == 1) enemies[i].y-=1;
                if(rnd == 3) enemies[i].y+=1;
            }

            // Gobbo goes for player
            if(enemies[i].tile == 'g' && is_within_range(player_x, player_y, enemies[i].x, enemies[i].y, 6)) {
                if(enemies[i].x > player_x) enemies[i].x-=1;
                if(enemies[i].x < player_x) enemies[i].x+=1;
                if(enemies[i].y > player_y) enemies[i].y-=1;
                if(enemies[i].y < player_y) enemies[i].y+=1;
            }

            // Redraw
            c=get_map(enemies[i].x,enemies[i].y);
            if((c!=' ' && c!='.') || c==enemies[i].tile) {
                enemies[i].x = enemies[i].old_x;
                enemies[i].y = enemies[i].old_y;
                if(c=='@') enemy_attack(i);
            }else{
                set_map(enemies[i].old_x, enemies[i].old_y, '.');
                cputcxy(enemies[i].old_x, enemies[i].old_y, '.');
            }

            if(enemies[i].x != enemies[i].old_x || enemies[i].y != enemies[i].old_y) {
                set_map(enemies[i].x, enemies[i].y, enemies[i].tile);
                cputcxy(enemies[i].x, enemies[i].y,enemies[i].tile);
            }
        }
    }

}


// Field of view/fog of war
void update_fov(int player_x, int player_y, int radius) {
    int dy,dx;
    for (dy = -radius; dy <= radius; dy++) {
        for (dx = -radius; dx <= radius; dx++) {
            int x = player_x + dx;
            int y = player_y + dy;

            // Ensure coordinates are within the map bounds
            if (x >= 0 && x < MAP_WIDTH-3 && y >= 0 && y < PLAYABLE_HEIGHT) {
                c=get_map(x, y);
                if(c==' ') c='.';
                cputcxy(x, y, c);
            }
        }
    }
}


void draw_screen() {

   
    // Draw whole screen
    int row,col;

    if(draw_whole_screen && screen_drawn == false) {
        for(row=0; row<PLAYABLE_HEIGHT; row++)
        {
            for(col=0; col < MAZE_WIDTH; col++){
                cputcxy(col,row,get_map(col,row));
            }
        };
        screen_drawn = true;
    } else {
       // Update the screen around the player with a set radius 
       update_fov(player_x, player_y, 2);
    }

}



void draw_momentary_object(unsigned int obj_old_x, unsigned int obj_old_y,unsigned int  obj_x, unsigned int obj_y, unsigned int obj_tile, unsigned int delay) {

    // Replace tile
    cputcxy(obj_old_x,obj_old_y,get_map(obj_old_x,obj_old_y));

    // Draw tile in new location
    cputcxy(obj_x,obj_y,obj_tile); 
   
    // Delay
    timer=dumb_wait(delay);

    // Replace tile again
    cputcxy(obj_x,obj_y,get_map(obj_x,obj_y));
    
}


void draw_move(bool replace) {

    // Delete the player character
    if(!replace) {
        set_map(old_x, old_y, '.');
    }

    // Draw new location
    cputcxy(old_x,old_y,get_map(old_x,old_y));
    cputcxy(player_x, player_y, 64); 
    set_map(player_x, player_y, 64);
}




// This is the default title screen
int title_screen() {
    
    clrscr();
    
    sprintf(output, "pet dungeon");
    cputsxy(11,10, output);
    
    sprintf(output, "a game by retrogamecoders.com");
    cputsxy(2,15, output);
    
    sprintf(output, "press a key");
    cputsxy(12,20, output);


    counter=0;
    while(!kbhit()) { counter++; }
    in_play=true;
    clrscr();
    return counter;
}


bool game_over() {
    clrscr();
    sprintf(output, " game over\n\n");
    cputsxy(15,10, output);
    refresh();
    timer=dumb_wait(1000);
    sprintf(output, " ah, such a shame,");
    cputsxy(16,12, output);
    sprintf(output, "you were doing so well!");
    cputsxy(16,14, output);
    timer=dumb_wait(1000);
    refresh();
    sprintf(output, " score:%03d",score);
    cputsxy(20,18, output);
    sprintf(output, " play again (y/n)?");
    cputsxy(20,19, output);
    refresh();
    key=cgetc();
    in_play=false;
    if(key=='n') {
        cursor(1);
        echo();
        refresh();
        return false;
    } else {
        return true;
    }
}

unsigned char get_key() {
            // Check keys;
        switch (key=cgetc()) 
        { 

            case 'w':
                if(player_y>0) player_y--; 
                break; 
            case 'a': 
                if(player_x>0) player_x--; 
                break; 
            
            case 's': 
                if(player_y<MAP_HEIGHT) player_y++; 
                break; 
            case 'd': 
                if(player_x<MAP_WIDTH) player_x++; 
                break; 


            case 'A': 
            case 'o':
                if(sword==true) {
                    draw_momentary_object(player_x-1,player_y,player_x-1,player_y,'-',2000); 
                    attack(10,player_x-1,player_y);
                }
                break;     

            case 'D': 
            case 'p':
                if(sword==true) {
                    draw_momentary_object(player_x+1,player_y,player_x+1,player_y,'-',2000); 
                    attack(10,player_x+1,player_y);
                }
                break; 
            case 'f': 

                if(magic > 5) {

                    magic-=5;
                    fx = player_x+direction_x;
                    fy = player_y+direction_y;  

                    c=get_map(fx,fy);
                    while((c==' '||c=='.') && magic > 0) {             
                        draw_momentary_object(fx,fy,fx,fy,'*',200); 
                        magic-=1;
                        fx = fx+direction_x;
                        fy = fy+direction_y;    
                        c=get_map(fx,fy);
                    }

                    attack(10,fx,fy);

                }

                break;            
            case 'Q':
            case 3:
                in_play = false;
                break;
            default:
                // Figure out scan codes 
                /*gotoxy(0,0);
                printf("%c %d",key, key);*/
                break; 
        }

        // debounce
        dumb_wait(100);

        return key;
}

void game_loop() {

        sprintf(output,"    k: %02d h: %03d *: %03d score: %04d", keys, health, magic, score);
        cputsxy(0,MAP_HEIGHT-1,output);
        refresh();

    // Change direction
    if(player_x != old_x || player_y != old_y) {
        direction_x = player_x-old_x;
        direction_y = player_y-old_y;
    }


    move_enemies();

    // Backup the location
    old_x = player_x;
    old_y = player_y;

    //if(kbhit()) { Remove comment to make more action than turn-based
        
    key = get_key();
    //} Remove comment to make more action than turn-based

    


    // Anything in our path?
    obstruction=false;
    c=get_map(player_x,player_y);
    
    // Collision
    switch (c)
    {
        case '#':
            // Wall               
            obstruction=true;
            break;
        
        // Spell
        case 's':
            draw_whole_screen=true;
            sprintf(output, "you activated a spell");
            break;

        case 'k': // Key +1
            keys+=1;
            sprintf(output, "you found a key");
            break;

        // Door
        case '+':
            if(keys>0)
            {
                keys-=1;
                score+=5;
                obstruction=false;
                sprintf(output, "you opened a door");

            }else{

                // Not enough keys to unlock!
                set_map(player_x, player_y, '-'); // turn into partially open
                health-=10; // lose 10 health
                obstruction=true;
                sprintf(output, "ouch!");
            }
            break;

        case '-': // Partially open door

            if(keys>0)
            {
                keys-=1;
                score+=5;
                obstruction=false;
                sprintf(output, "you opened a door");

            }else{
                // Not enough keys to unlock!
                set_map(player_x, player_y, '.');  // turn into fully open
                health-=10;         // lose 10 health
                obstruction=true;
                sprintf(output, "who needs keys anyway?");
                
            }
            break;

        case '|': // Sword!
            sword=true;
            
            if(weapon<5) {
                weapon=5;
                sprintf(output, "you found a sword!");
            
            } else {
                weapon++;
                sprintf(output, "+1 to your attack!");
            }


            break;

        case '$': // Cash money
            score+=5;
            sprintf(output, "ka-ching!");
            break;

        case '*': // Potion
            score+=15;
            magic+=100;
            sprintf(output, "power up!");
            break;

        case 'm': // Cash money
            score+=15;
            break;

        case 'h': // Health
            health+=25;
            sprintf(output, "ahh that is better!");
            if(health>100) health=100; // Can't be more than 100%!
            break;


/* Enemies >> */

        case 'g': // Gobbo
            attack(weapon,player_x,player_y);
            obstruction=true;
            break;

        case 'r': // Rats
            attack(weapon,player_x,player_y);
            obstruction=true;
            break;

/* ^^ Enemies */

        case 'i': // Idol
            score+=10;
            idols+=1;
            if(idols==room) {
                room+=1;
                load_room();
                draw_screen();
                idols=0;
            }
            break;

        case 0:
        case 64: // Player
            break;
        
        default:
            
            // convert integer to ascii: itoa(c,buffer,10);
            
            if(c!=' ' && c!='.') {
                // Figure out what the code is for tile
                gotoxy(0,0);
                sprintf(output, "bumped into ...... %03d",c);
                obstruction=true;
            }
            
            break;
    }

    if(output[1]> 32) {
        // Update message box
        output_message();
        timer=dumb_wait(1000);
    }

    // If obstructed then bounce
    if(obstruction) {
        player_x=old_x;
        player_y=old_y;
    } else {
        draw_move(false);
        draw_screen();
    }

    if(health<1) {
        in_play = false;
    }

    
    
}


int main() {



    /* Clear Screen */
    clrscr();

    /* Hide cursor */
    cursor(0);

    // Titles
    counter = title_screen();
    init();

    // Start running     
    run=true;
        
    // Should the program end?
    while(run){

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
        cputcxy(player_x,player_y,'@');

        // Game on!
        in_play = true;

        /* Loop until game ends */
        while (in_play)
        {
            game_loop();
        }

        // Try again or quit?
        run=game_over();

    }
    clrscr();
    sprintf(output, "goodbye!\n\n");
    output_message();
    cursor(1);
    return(0);
}
