#ifndef MAZE_H
#define MAZE_H

#include "globals.h"

// Maze generation functions
void carveMaze(void);
void placePlayer(void);
void placeObject(unsigned char tile);
void placeSkeleton(unsigned char tile);
void placeHDoor(void);
void placeVDoor(void);

// Global maze array
extern unsigned char map[MAP_HEIGHT][MAP_WIDTH + 1];

// Structure for a cell coordinate in the grid
typedef struct {
    int x, y;
} Cell;

// Global stack for DFS (max size: CELLS_X * CELLS_Y)
extern Cell stack[CELLS_X * CELLS_Y];
extern int stackSize;

// Global visited array for the cell grid
extern int visited[CELLS_Y][CELLS_X];

// Stack operations
void push(const Cell *this_cell);
void pop(Cell *out_cell);
int isEmpty(void);
void shuffle(int *array, int n);

#ifdef __CC65__
typedef unsigned long time_t;
time_t time(time_t* t);
#endif

#endif /* MAZE_H */ 