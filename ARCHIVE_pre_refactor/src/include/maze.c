#include "maze.h"
#include <stdlib.h>

#ifndef __CC65__
#include <time.h>
#endif
// Global maze array
unsigned char map[MAP_HEIGHT][MAP_WIDTH + 1];

// Global stack for DFS (max size: CELLS_X * CELLS_Y)
Cell stack[CELLS_X * CELLS_Y];
int stackSize = 0;

// Global visited array for the cell grid
int visited[CELLS_Y][CELLS_X];

#ifdef __CC65__
time_t time(time_t* t) {
    time_t current_time = rand() % (100000000);
    if (t != NULL) {
        *t = current_time;
    }
    return current_time;
}
#endif

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

/* Place an object in a random valid position within the playable area.
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

/* Place a skeleton in a random valid position with extra space around it for patrolling.
   The candidate cell must be a '.' with '.' in a larger area around it. */
void placeSkeleton(unsigned char tile) {
    // Just use the same approach as placeObject for now
    placeObject(tile);
}

void placeHDoor(void) {
    int row, col;
    unsigned char tile;
    tile = '+';
    do {
        row = (rand() % (PLAYABLE_HEIGHT - 2)) + HUD_TOP + 1;
        col = (rand() % (MAP_WIDTH - 2)) + 1;
    } while (map[row][col] != '.' ||
             map[row][col-1] != '.' || map[row][col-2] != '#' ||
             map[row][col+1] != '.' || map[row][col+2] != '#' ||
             map[row-1][col] != '.' || map[row+1][col] != '.' ||
             map[row-2][col] != '.' || map[row+2][col] != '.');
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
             map[row-1][col] != '.' ||
             map[row-2][col] != '#' ||
             map[row-1][col] != '.' ||
             map[row+2][col] != '#' ||
             map[row][col+1] != '.' ||
             map[row][col-1] != '.' ||
             map[row][col+2] != '.' ||
             map[row][col-2] != '.');
    map[row][col] = tile;
    map[row-1][col] = '#';
    map[row+1][col] = '#';
} 