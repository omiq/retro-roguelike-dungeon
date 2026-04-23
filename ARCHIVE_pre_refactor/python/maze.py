import pygame
import random
import time

# Constants
MAP_WIDTH = 40
MAP_HEIGHT = 24
HUD_TOP = 1
HUD_BOTTOM = 2
PLAYABLE_HEIGHT = MAP_HEIGHT - HUD_TOP - HUD_BOTTOM
CELLS_X = (MAP_WIDTH - 1) // 4
CELLS_Y = (PLAYABLE_HEIGHT - 1) // 4
MAZE_WIDTH = CELLS_X * 4 + 1
MAZE_HEIGHT = CELLS_Y * 4 + 1
CELL_WIDTH = 3
CELL_HEIGHT = 3
TILE_SIZE = 20

# Global final map (full screen, including HUD)
map = [['#' for _ in range(MAP_WIDTH + 1)] for _ in range(MAP_HEIGHT)]

# Structure for a cell coordinate in the grid
class Cell:
    def __init__(self, x, y):
        self.x = x
        self.y = y

# Global stack for DFS (max size: CELLS_X * CELLS_Y)
stack = []
stackSize = 0

# Global visited array for the cell grid
visited = [[0 for _ in range(CELLS_X)] for _ in range(CELLS_Y)]

# Push a cell onto the stack
def push(this_cell):
    global stackSize
    stack.append(this_cell)
    stackSize += 1

# Pop a cell from the stack
def pop():
    global stackSize
    stackSize -= 1
    return stack.pop()

# Check if the stack is empty
def isEmpty():
    return stackSize == 0

# Shuffle an array of integers in-place
def shuffle(array):
    random.shuffle(array)

# Maze-generation algorithm using DFS (recursive backtracker)
def carveMaze():
    global map, visited

    # Initialize the temporary maze with walls
    for i in range(MAZE_HEIGHT):
        for j in range(MAZE_WIDTH):
            map[i][j] = '#'
        map[i][MAZE_WIDTH] = '\0'

    # Clear the visited array for the cell grid
    for i in range(CELLS_Y):
        for j in range(CELLS_X):
            visited[i][j] = 0

    # Start cell (0, 0)
    startX, startY = 0, 0
    visited[startY][startX] = 1
    sx, sy = startX * 4 + 1, startY * 4 + 1
    for i in range(CELL_HEIGHT):
        for j in range(CELL_WIDTH):
            map[sy + i][sx + j] = '.'
    push(Cell(startX, startY))

    # DFS to carve the maze
    while not isEmpty():
        current = stack[-1]  # Peek at the top
        directions = [0, 1, 2, 3]
        carved = False
        shuffle(directions)
        for dir in directions:
            nx, ny = current.x, current.y
            if dir == 0:
                ny -= 1  # Up
            elif dir == 1:
                nx += 1  # Right
            elif dir == 2:
                ny += 1  # Down
            elif dir == 3:
                nx -= 1  # Left
            if nx < 0 or nx >= CELLS_X or ny < 0 or ny >= CELLS_Y:
                continue
            if visited[ny][nx]:
                continue

            cx, cy = current.x * 4 + 1, current.y * 4 + 1
            nx_maze, ny_maze = nx * 4 + 1, ny * 4 + 1
            if dir == 0:  # Up
                wallY = cy - 1
                for j in range(CELL_WIDTH):
                    map[wallY][cx + j] = '.'
            elif dir == 1:  # Right
                wallX = cx + CELL_WIDTH
                for i in range(CELL_HEIGHT):
                    map[cy + i][wallX] = '.'
            elif dir == 2:  # Down
                wallY = cy + CELL_HEIGHT
                for j in range(CELL_WIDTH):
                    map[wallY][cx + j] = '.'
            elif dir == 3:  # Left
                wallX = cx - 1
                for i in range(CELL_HEIGHT):
                    map[cy + i][wallX] = '.'
            for i in range(CELL_HEIGHT):
                for j in range(CELL_WIDTH):
                    map[ny_maze + i][nx_maze + j] = '.'
            visited[ny][nx] = 1
            push(Cell(nx, ny))
            carved = True
            break
        if not carved:
            pop()

# Place the player character '@' in a random valid position within the playable area.
def placePlayer():
    while True:
        row = random.randint(HUD_TOP + 1, PLAYABLE_HEIGHT - 2)
        col = random.randint(1, MAP_WIDTH - 2)
        if (map[row][col] == '.' and
            map[row-1][col] == '.' and
            map[row+1][col] == '.' and
            map[row][col-1] == '.' and
            map[row][col+1] == '.'):
            map[row][col] = '@'
            break

# Place an object in a random valid position within the playable area.
def placeObject(tile):
    while True:
        row = random.randint(HUD_TOP + 1, PLAYABLE_HEIGHT - 2)
        col = random.randint(1, MAP_WIDTH - 2)
        if (map[row][col] == '.' and
            map[row-1][col] == '.' and
            map[row+1][col] == '.' and
            map[row][col-1] == '.' and
            map[row][col+1] == '.'):
            map[row][col] = tile
            break

# Place a horizontal door in a random valid position within the playable area.
def placeHDoor():
    tile = '+'
    while True:
        row = random.randint(HUD_TOP + 1, PLAYABLE_HEIGHT - 2)
        col = random.randint(1, MAP_WIDTH - 2)
        if (map[row][col] == '.' and
            map[row][col-1] == '.' and map[row][col-2] == '#' and
            map[row][col+1] == '.' and map[row][col+2] == '#' and
            map[row-1][col] == '.' and map[row+1][col] == '.' and
            map[row-2][col] == '.' and map[row+2][col] == '.'):
            map[row][col] = tile
            map[row][col-1] = '#'
            map[row][col+1] = '#'
            break

# Place a vertical door in a random valid position within the playable area.
def placeVDoor():
    tile = '+'
    while True:
        row = random.randint(HUD_TOP + 1, PLAYABLE_HEIGHT - 2)
        col = random.randint(1, MAP_WIDTH - 2)
        if (map[row][col] == '.' and
            map[row-1][col] == '.' and map[row-2][col] == '#' and
            map[row+1][col] == '.' and map[row+2][col] == '#' and
            map[row][col+1] == '.' and map[row][col-1] == '.' and
            map[row][col+2] == '.' and map[row][col-2] == '.'):
            map[row][col] = tile
            map[row-1][col] = '#'
            map[row+1][col] = '#'
            break

def draw_map(screen):
    colors = {
        '#': (0, 0, 0),  # Black for walls
        '.': (255, 255, 255),  # White for paths
        '@': (255, 0, 0),  # Red for player
        '+': (0, 255, 0)  # Green for doors
    }
    for y in range(MAP_HEIGHT):
        for x in range(MAP_WIDTH):
            tile = map[y][x]
            color = colors.get(tile, (0, 0, 0))
            pygame.draw.rect(screen, color, pygame.Rect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE))

def main():
    pygame.init()
    screen = pygame.display.set_mode((MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE))
    pygame.display.set_caption("Maze Game")

    carveMaze()
    placePlayer()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        screen.fill((0, 0, 0))
        draw_map(screen)
        pygame.display.flip()

    pygame.quit()

if __name__ == "__main__":
    main()