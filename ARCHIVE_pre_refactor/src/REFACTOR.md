# Dungeon Game Code Reorganization

This document explains the reorganization of the Dungeon game codebase into a modular structure, along with key C programming concepts used in the process.

## Table of Contents

1. [Project Structure](#project-structure)
2. [C Programming Concepts](#c-programming-concepts)
   - [Header (.h) vs Implementation (.c) Files](#header-h-vs-implementation-c-files)
   - [Global Variables and the `extern` Keyword](#global-variables-and-the-extern-keyword)
   - [Include Guards](#include-guards)
3. [The Build System](#the-build-system)
   - [Understanding the Makefile](#understanding-the-makefile)
   - [Compilation Process](#compilation-process)
4. [Code Organization](#code-organization)
   - [Component Breakdown](#component-breakdown)
   - [Dependencies Between Components](#dependencies-between-components)

## Project Structure

The codebase has been reorganized into the following directory structure:

```
src/
├── display/           # Display and rendering functions
│   ├── display.c
│   └── display.h
├── include/           # Common includes and global definitions
│   ├── globals.c
│   ├── globals.h
│   ├── maze.c
│   ├── maze.h
│   ├── notconio.c
│   └── notconio.h
├── input/             # Input handling functions
│   ├── input.c
│   └── input.h
├── logic/             # Game logic and mechanics
│   ├── game_logic.c
│   └── game_logic.h
├── screens/           # Title and game over screens
│   ├── screens.c
│   └── screens.h
├── main.c             # Main program entry point
└── Makefile           # Build system configuration
```

This structure separates the code into logical components, making it easier to understand, maintain, and extend.

## C Programming Concepts

### Header (.h) vs Implementation (.c) Files

In C, code is typically split between header files (`.h`) and implementation files (`.c`):

#### Header Files (.h)

Header files contain:
- Function declarations (prototypes)
- Type definitions (structs, enums, typedefs)
- Constant definitions
- Macro definitions
- External variable declarations

They serve as an interface or contract for what functionality a module provides. For example, in `display.h`:

```c
// Function declarations
void output_message(void);
void draw_screen(void);
void draw_momentary_object(unsigned int obj_old_x, unsigned int obj_old_y, 
                          unsigned int obj_x, unsigned int obj_y, 
                          unsigned int obj_tile, unsigned int delay);
```

#### Implementation Files (.c)

Implementation files contain:
- The actual code that implements the functions declared in the header
- Definition of static (file-scope) variables
- Definition of global variables

For example, in `display.c`:

```c
void draw_screen(void) {
    // Draw whole screen
    int row, col;

    if (draw_whole_screen && screen_drawn == false) {
        for (row = 0; row < PLAYABLE_HEIGHT; row++) {
            for (col = 0; col < MAZE_WIDTH; col++) {
                cputcxy(col, row, get_map(col, row));
            }
        }
        screen_drawn = true;
    } else {
        // Update the screen around the player with a set radius 
        update_fov(player_x, player_y, 2);
    }
}
```

#### Benefits of Separation

1. **Encapsulation**: Implementation details are hidden from users of the module
2. **Compilation efficiency**: Changes to implementation don't require recompiling all files that use the module
3. **Clarity**: Clear separation between interface and implementation
4. **Reduced conflicts**: Minimizes naming conflicts and unintended dependencies

### Global Variables and the `extern` Keyword

Global variables are variables that can be accessed from any part of the program. In a modular codebase, they need to be:

1. **Defined** in exactly one `.c` file
2. **Declared** with the `extern` keyword in a header file

#### Example from Our Codebase

In `globals.h`, we declare global variables:

```c
// Game state variables
extern bool run;
extern bool in_play;
extern bool obstruction;
extern bool screen_drawn;
extern bool draw_whole_screen;

// Player variables
extern unsigned char player_x;
extern unsigned char player_y;
// ... more variables ...
```

In `globals.c`, we define these variables:

```c
// Game state variables
bool run = true;
bool in_play = false;
bool obstruction = false;
bool screen_drawn = false;
bool draw_whole_screen = false;

// Player variables
unsigned char player_x = 19;
unsigned char player_y = 8;
// ... more variables ...
```

#### The `extern` Keyword

The `extern` keyword tells the compiler "this variable is defined somewhere else." It's a promise that the linker will find the actual definition in another compilation unit.

Benefits:
- Prevents multiple definitions of the same variable
- Allows sharing variables across multiple files
- Maintains a single source of truth for variable initialization

### Include Guards

Include guards prevent a header file from being included multiple times in the same compilation unit, which would cause redefinition errors.

Example from `globals.h`:

```c
#ifndef GLOBALS_H
#define GLOBALS_H

// Header content goes here...

#endif /* GLOBALS_H */
```

How it works:
1. First time the file is included: `GLOBALS_H` is not defined, so the preprocessor includes the content
2. `GLOBALS_H` is defined
3. Subsequent inclusions: `GLOBALS_H` is already defined, so the content is skipped

## The Build System

### Understanding the Makefile

A Makefile is a script used by the `make` utility to build the program. Our Makefile contains:

```makefile
CC = gcc                # Compiler to use
CFLAGS = -Wall -Wextra -g  # Compiler flags
LDFLAGS = -lncurses     # Linker flags

# Source files
SRC = main.c \
      include/globals.c \
      include/maze.c \
      include/notconio.c \
      display/display.c \
      input/input.c \
      logic/game_logic.c \
      screens/screens.c

# Object files
OBJ = $(SRC:.c=.o)      # Replace .c with .o for all source files

# Executable name
TARGET = dungeon_modular

all: $(TARGET)          # Default target

$(TARGET): $(OBJ)       # Link object files to create executable
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c                # Compile source files into object files
	$(CC) $(CFLAGS) -c $< -o $@

clean:                  # Remove generated files
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean       # Declare targets that don't create files
```

### Compilation Process

The compilation process has multiple stages:

1. **Preprocessing**: Expands macros, includes header files
2. **Compilation**: Converts C code to assembly
3. **Assembly**: Converts assembly to machine code (object files)
4. **Linking**: Combines object files and libraries into an executable

In our Makefile:
- `%.o: %.c` rule handles steps 1-3 for each source file
- `$(TARGET): $(OBJ)` rule handles step 4

#### Special Variables in Makefiles

- `$@`: Target of the rule (left side of the colon)
- `$^`: All prerequisites (right side of the colon)
- `$<`: First prerequisite

#### Building the Project

To build the project:
```bash
make
```

To clean generated files:
```bash
make clean
```

## Code Organization

### Component Breakdown

The codebase is organized into logical components:

1. **Global Variables and Structs** (`include/globals.h`, `include/globals.c`)
   - Contains all global variables used across the game
   - Defines common structures like the enemy struct

2. **Display/Output** (`display/display.h`, `display/display.c`)
   - Functions for rendering the game world
   - Screen update and message display

3. **Input/Controls** (`input/input.h`, `input/input.c`)
   - Keyboard input handling
   - Timing functions

4. **Game Logic** (`logic/game_logic.h`, `logic/game_logic.c`)
   - Core game mechanics
   - Enemy AI and combat
   - Map manipulation

5. **Screens** (`screens/screens.h`, `screens/screens.c`)
   - Title screen
   - Game over screen

6. **Maze Generation** (`include/maze.h`, `include/maze.c`)
   - Procedural maze generation
   - Object placement

7. **Console I/O** (`include/notconio.h`, `include/notconio.c`)
   - Platform-independent console functions
   - Abstraction layer for terminal operations

### Dependencies Between Components

Components have dependencies on each other, represented by `#include` statements:

- **Main** depends on all components
- **Display** depends on Game Logic and Input
- **Game Logic** depends on Display, Input, and Maze
- **Input** depends on Game Logic and Display
- **Screens** depends on Display and Input

This dependency graph helps understand how the components interact and ensures proper initialization order.

## Conclusion

The reorganized codebase is now more modular, maintainable, and easier to understand. Each component has a clear responsibility, and the dependencies between components are explicit. This structure makes it easier to extend the game with new features or fix bugs without affecting unrelated parts of the code. 