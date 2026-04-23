#include "globals.h"

// Game state variables
bool run = true;
bool in_play = false;
bool obstruction = false;
bool screen_drawn = false;
bool draw_whole_screen = false;

char reverse_on[40]="\033[7m";
char reverse_off[40]="\033[m";


// Player variables
unsigned char player_x = 19;
unsigned char player_y = 8;
unsigned char old_x = 0;
unsigned char old_y = 0;
unsigned char direction_x = 0;
unsigned char direction_y = 0;
unsigned char fx = 0;
unsigned char fy = 0;
unsigned char keys = 0;
unsigned char idols = 0;
unsigned char potion = 0;
signed char health = 100;
int magic = 0;
unsigned int score = 0;
bool sword = false;
unsigned char weapon = 1;
unsigned char room = 2;

// Utility variables
unsigned int this_row = 0;
unsigned int this_col = 0;
unsigned int timer = 0;
unsigned int delay = 0;
unsigned int counter = 0;
unsigned char key = 0;
unsigned char i = 0;
unsigned char c = 0;
char output[40];

// Enemy variables
struct enemy enemies[25];
unsigned int enemy_count = 0;
unsigned int this_enemy = 0;

// Map variables
unsigned char game_map[1000];
unsigned char info_row = 22; 