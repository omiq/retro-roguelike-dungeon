import pygame
import random
import time
from maze import carveMaze, placePlayer, placeObject, placeHDoor, placeVDoor, map
from game_objects import load_images, Enemy

# Constants
MAP_WIDTH = 40
MAP_HEIGHT = 24
HUD_TOP = 1
HUD_BOTTOM = 2
PLAYABLE_HEIGHT = MAP_HEIGHT - HUD_TOP - HUD_BOTTOM
TILE_SIZE = 20

# Global variables
run = True
in_play = False
obstruction = False

player_x = -1
player_y = -1
old_x, old_y = 0, 0
direction_x, direction_y = 0, 0
fx, fy = 0, 0
room = 2
draw_whole_screen = False

keys, idols, potion = 0, 0, 0
health = 100
magic = 0
score = 0
sword = False
weapon = 1

enemy_count = 0
enemies = []

# Initialize the game
def init():
    global keys, health, score, room, potion, magic, enemy_count, sword, weapon, idols, draw_whole_screen
    keys = 0
    health = 100
    score = 0
    room = 2
    potion = 0
    magic = 0
    enemy_count = 0
    sword = False
    weapon = 1
    idols = 0
    draw_whole_screen = False

# Load the room
def load_room():
    global player_x, player_y, enemy_count, enemies, map, room, idols
    carveMaze()
    placePlayer()
    enemies = []
    enemy_count = 0
    idols = 0

    for i in range(room - 1):
        placeObject('g')
    for i in range(room):
        placeObject('i')
    for i in range(room + 1):
        placeObject('r')
    placeObject('*')
    placeObject('|')
    placeObject('$')
    placeObject('h')
    placeObject('k')
    placeObject('s')
    placeHDoor()
    placeVDoor()

    for y in range(PLAYABLE_HEIGHT):
        for x in range(MAP_WIDTH):
            c = map[y][x]
            if c == '@':
                player_y = y
                player_x = x
            elif c == 'g':
                enemies.append(Enemy(c, room, x, y, 30, 5, 1, 10))
                enemy_count += 1
            elif c == 'r':
                enemies.append(Enemy(c, room, x, y, 15, 5, 2, 0))

# Draw the map
def draw_map(screen, images):
    for y in range(PLAYABLE_HEIGHT):
        for x in range(MAP_WIDTH-3):
            tile = map[y][x]
            if tile in images:
                screen.blit(images[tile], (x * TILE_SIZE, y * TILE_SIZE))

# Check if the position is walkable
def is_walkable(x, y):
    return map[y][x] in ['.', '@']

# Move the player
def move_player(dx, dy):
    global player_x, player_y, health, score, keys, idols, potion, sword, weapon, magic, room
    new_x = player_x + dx
    new_y = player_y + dy
    if is_walkable(new_x, new_y):
        # Clear the player's old position
        map[player_y][player_x] = '.'
        player_x = new_x
        player_y = new_y
        # Update the player's new position
        map[player_y][player_x] = '@'
        print(f"Player moved to ({player_x}, {player_y})")  # Debug print
        # Handle interactions with objects
        tile = map[player_y][player_x]
        if tile == 'i':
            idols += 1
            if idols == room:
                room += 1
                load_room()
            map[player_y][player_x] = '@'
        elif tile == '$':
            score += 10
            map[player_y][player_x] = '@'
        elif tile == 'h':
            health += 10
            map[player_y][player_x] = '@'
        elif tile == 'k':
            keys += 1
            map[player_y][player_x] = '@'
        elif tile == '|':
            if not sword:
                sword = True
            else:
                weapon += 1
            map[player_y][player_x] = '@'
        elif tile == '*':
            magic += 10
            map[player_y][player_x] = '@'

# Move enemies
def move_enemies():
    global health, run
    for enemy in enemies:
        if enemy.health <= 0:
            continue
        dx = player_x - enemy.x
        dy = player_y - enemy.y
        if abs(dx) > abs(dy):
            step_x = 1 if dx > 0 else -1
            step_y = 0
        else:
            step_x = 0
            step_y = 1 if dy > 0 else -1
        new_x = enemy.x + step_x
        new_y = enemy.y + step_y
        if is_walkable(new_x, new_y):
            map[enemy.y][enemy.x] = '.'  # Clear the enemy's old position
            enemy.old_x = enemy.x
            enemy.old_y = enemy.y
            enemy.x = new_x
            enemy.y = new_y
            map[enemy.y][enemy.x] = enemy.tile  # Update the enemy's new position
        # Check for combat with player
        if enemy.x == player_x and enemy.y == player_y:
            health -= enemy.strength
            if health > 0:
                print(f"Player attacked by {enemy.tile} at ({enemy.x}, {enemy.y}). Health: {health}")  # Debug print
            else:
                run = False

# Main game loop
def game_loop(screen, images):
    global player_x, player_y, old_x, old_y, direction_x, direction_y, health, score, magic, sword, weapon, keys, idols, room, in_play, run
    pressed = True        
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                run = False
            elif event.type == pygame.KEYDOWN:
                pressed = True
                if event.key == pygame.K_w:
                    move_player(0, -1)
                elif event.key == pygame.K_a:
                    move_player(-1, 0)
                elif event.key == pygame.K_s:
                    move_player(0, 1)
                elif event.key == pygame.K_d:
                    move_player(1, 0)
                elif event.key == pygame.K_q:
                    in_play = False
                    run = False

        if pressed: 
            move_enemies()
            screen.fill((0, 0, 0))
            draw_map(screen, images)
            pygame.display.flip()
            time.sleep(0.1)
            pressed = False        


# Main function
def main():
    pygame.init()
    screen = pygame.display.set_mode((MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE))
    pygame.display.set_caption("Dungeon Game")

    images = load_images()
    init()
    load_room()

    game_loop(screen, images)
    pygame.quit()

if __name__ == "__main__":
    main()