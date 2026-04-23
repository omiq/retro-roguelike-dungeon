import os
import pygame
from maze import TILE_SIZE

# Load images for each character
def load_images():
    images = {}
    characters = ['#', '.', '@', 'g', 'r', 'i', '*', '|', '$', 'h', 'k', 's', '+', '-']
    for char in characters:
        filename = f'{char}.png'
        if not os.path.exists(filename):
            create_placeholder_image(char, filename)
        images[char] = pygame.image.load(filename)
    return images

# Create placeholder images for characters
def create_placeholder_image(char, filename):
    font = pygame.font.SysFont('Arial', TILE_SIZE)
    image = pygame.Surface((TILE_SIZE, TILE_SIZE))
    image.fill((255, 255, 255))
    text_surface = font.render(char, True, (0, 0, 0))
    image.blit(text_surface, (0, 0))
    pygame.image.save(image, filename)

# Structure for a cell coordinate in the grid
class Cell:
    def __init__(self, x, y):
        self.x = x
        self.y = y

# Structure for an enemy
class Enemy:
    def __init__(self, tile, room, x, y, health, strength, speed, armour):
        self.tile = tile
        self.room = room
        self.x = x
        self.y = y
        self.old_x = x
        self.old_y = y
        self.health = health
        self.strength = strength
        self.speed = speed
        self.armour = armour