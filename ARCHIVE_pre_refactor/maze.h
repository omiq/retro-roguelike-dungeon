#include <stdio.h>
#include <stdlib.h>

#if defined (__CC65__)
typedef unsigned long time_t;
time_t time(time_t* t) {
    t=0;
    return rand() % (100000000);
}


#else
#include <time.h>

#endif

// min screen
    #define MAP_WIDTH 40
    #define MAP_HEIGHT 24
    unsigned char info_row = 22;



/* Reserve top 1 and bottom 2 lines for HUD */
#define HUD_TOP 1
#define HUD_BOTTOM 2
#define PLAYABLE_HEIGHT (MAP_HEIGHT - HUD_TOP - HUD_BOTTOM)

/* Define cell grid dimensions based on the playable area */
#define CELLS_X ((MAP_WIDTH - 1) / 4)         /* For 40: (39/4)=9 cells horizontally */
#define CELLS_Y ((PLAYABLE_HEIGHT - 1) / 4)     /* For 21: (20/4)=5 cells vertically */

/* Maze dimensions after expansion from the cell grid */
#define MAZE_WIDTH  (CELLS_X * 4 + 1)  /* e.g., 9 * 4 + 1 = 37 */
#define MAZE_HEIGHT (CELLS_Y * 4 + 1)  /* e.g., 5 * 4 + 1 = 21 */

/* Passage dimensions in each cell */
#define CELL_WIDTH  3
#define CELL_HEIGHT 3

/* Global final map (full screen, including HUD) */
unsigned char map[MAP_HEIGHT][MAP_WIDTH + 1];

/* Global temporary maze array */
//unsigned char maze[MAZE_HEIGHT][MAZE_WIDTH + 1];

/* Structure for a cell coordinate in the grid */
typedef struct {
    int x, y;
} Cell;

/* Global stack for DFS (max size: CELLS_X * CELLS_Y) */
Cell stack[CELLS_X * CELLS_Y];
int stackSize = 0;

/* Global visited array for the cell grid */
int visited[CELLS_Y][CELLS_X];

/* Push a cell onto the stack */
void push(const Cell *this_cell) {
    stack[stackSize] = *this_cell;
    stackSize++;
}

/* Pop a cell from the stack */
void pop(Cell *out_cell) {
    stackSize--;
    *out_cell = stack[stackSize];
}

/* Check if the stack is empty */
int isEmpty(void) {
    return (stackSize == 0);
}

/* Shuffle an array of integers in-place */
void shuffle(int *array, int n) {
    int i, j, temp;
    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

/* Maze-generation algorithm using DFS (recursive backtracker) */
void carveMaze(void) {
    int i, j, k;

    /* Initialize the temporary maze with walls */
    for (i = 0; i < MAZE_HEIGHT; i++) {
        for (j = 0; j < MAZE_WIDTH; j++) {
            map[i][j] = '#';
        }
        map[i][MAZE_WIDTH] = '\0';
    }

    /* Clear the visited array for the cell grid */
    for (i = 0; i < CELLS_Y; i++) {
        for (j = 0; j < CELLS_X; j++) {
            visited[i][j] = 0;
        }
    }

    /* Start cell (0, 0) */
    {
        int startX, startY, sx, sy;
        startX = 0;
        startY = 0;
        visited[startY][startX] = 1;
        sx = startX * 4 + 1;
        sy = startY * 4 + 1;
        for (i = 0; i < CELL_HEIGHT; i++) {
            for (j = 0; j < CELL_WIDTH; j++) {
                map[sy + i][sx + j] = '.';
            }
        }
        {
            Cell this_c;
            this_c.x = startX;
            this_c.y = startY;
            push(&this_c);
        }
    }

    /* DFS to carve the maze */
    while (!isEmpty()) {
        Cell current;
        int directions[4];  
        int carved;
        current = stack[stackSize - 1];  /* Peek at the top */
        directions[0] = 0;
        directions[1] = 1;
        directions[2] = 2;
        directions[3] = 3;
        carved = 0;
        shuffle(directions, 4);
        for (k = 0; k < 4; k++) {
            int dir, nx, ny;
            dir = directions[k];
            nx = current.x;
            ny = current.y;
            if (dir == 0) {
                ny--;  /* Up */
            } else if (dir == 1) {
                nx++;  /* Right */
            } else if (dir == 2) {
                ny++;  /* Down */
            } else if (dir == 3) {
                nx--;  /* Left */
            }
            if (nx < 0 || nx >= CELLS_X || ny < 0 || ny >= CELLS_Y)
                continue;
            if (visited[ny][nx])
                continue;

            {
                int cx, cy, nx_maze, ny_maze;
                cx = current.x * 4 + 1;
                cy = current.y * 4 + 1;
                nx_maze = nx * 4 + 1;
                ny_maze = ny * 4 + 1;
                if (dir == 0) {  /* Up */
                    int wallY = cy - 1;
                    for (j = 0; j < CELL_WIDTH; j++) {
                        map[wallY][cx + j] = '.';
                    }
                } else if (dir == 1) {  /* Right */
                    int wallX = cx + CELL_WIDTH;
                    for (i = 0; i < CELL_HEIGHT; i++) {
                        map[cy + i][wallX] = '.';
                    }
                } else if (dir == 2) {  /* Down */
                    int wallY = cy + CELL_HEIGHT;
                    for (j = 0; j < CELL_WIDTH; j++) {
                        map[wallY][cx + j] = '.';
                    }
                } else if (dir == 3) {  /* Left */
                    int wallX = cx - 1;
                    for (i = 0; i < CELL_HEIGHT; i++) {
                        map[cy + i][wallX] = '.';
                    }
                }
                for (i = 0; i < CELL_HEIGHT; i++) {
                    for (j = 0; j < CELL_WIDTH; j++) {
                        map[ny_maze + i][nx_maze + j] = '.';
                    }
                }
            }
            visited[ny][nx] = 1;
            {
                Cell this_c;
                this_c.x = nx;
                this_c.y = ny;
                push(&this_c);
            }
            carved = 1;
            break;
        }
        if (!carved) {
            pop(&current);
        }
    }




}

/* Place the player character '@' in a random valid position within the playable area.
   The candidate cell must be a '.' with '.' immediately above, below, left, and right. */
void placePlayer(void) {
    int row, col;
    do {
        row = (rand() % (PLAYABLE_HEIGHT - 2)) + HUD_TOP + 1;
        col = (rand() % (MAP_WIDTH - 2)) + 1;
    } while (map[row][col] != '.' ||
             map[row-1][col] != '.' ||
             map[row+1][col] != '.' ||
             map[row][col-1] != '.' ||
             map[row][col+1] != '.');
    map[row][col] = '@';
}

/* Place the player character '@' in a random valid position within the playable area.
   The candidate cell must be a '.' with '.' immediately above, below, left, and right. */
void placeObject(unsigned char tile) {
    int row, col;
    do {
        row = (rand() % (PLAYABLE_HEIGHT - 2)) + HUD_TOP + 1;
        col = (rand() % (MAP_WIDTH - 2)) + 1;
    } while (map[row][col] != '.' ||
             map[row-1][col] != '.' ||
             map[row+1][col] != '.' ||
             map[row][col-1] != '.' ||
             map[row][col+1] != '.');
    map[row][col] = tile;
}

void placeHDoor(void) {
    int row, col;
    unsigned char tile;
    tile = '+';
    do {
        row = (rand() % (PLAYABLE_HEIGHT - 2)) + HUD_TOP + 1;
        col = (rand() % (MAP_WIDTH - 2)) + 1;
    } while (   map[row][col] != '.'   ||
                map[row][col-1] != '.' || map[row][col-2] != '#' ||
                map[row][col+1] != '.' || map[row][col+2] != '#' ||
                map[row-1][col] != '.' || map[row+1][col] != '.' ||
                map[row-2][col] != '.' || map[row+2][col] != '.'     
            );
    map[row][col] = tile;
    map[row][col-1] = '#';
    map[row][col+1] = '#';
}

void placeVDoor(void) {
    int row, col;
    unsigned char tile;
    tile = '+';
    do {
        row = (rand() % (PLAYABLE_HEIGHT - 2)) + HUD_TOP + 1;
        col = (rand() % (MAP_WIDTH - 2)) + 1;
    } while (map[row][col] != '.' ||
             map[row-1][col] != '.'||
             map[row-2][col] != '#'||
             map[row-1][col] != '.'||
             map[row+2][col] != '#'||
             map[row][col+1] != '.'||
             map[row][col-1] != '.'||
             map[row][col+2] != '.'||
             map[row][col-2] != '.'          
            
            );
    map[row][col] = tile;
    map[row-1][col] = '#';
    map[row+1][col] = '#';
}
