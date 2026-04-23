# GCC is the compiler for modern desktop systems
# CL65 is the compiler for the Commodore 64 and PET
# Eg. To compile for PET we need to use the following command:
# cl65 -t pet -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o dungeonPET.prg dungeon.c

# Compiler definitions
CC = gcc
CL65 = cl65
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses

# Common source files
COMMON_SRC = main.c \
             include/globals.c \
             include/maze.c \
             display/display.c \
             input/input.c \
             logic/game_logic.c \
             screens/screens.c

# Desktop-specific source files
DESKTOP_SRC = $(COMMON_SRC) include/notconio.c

# Object files for desktop
DESKTOP_OBJ = $(DESKTOP_SRC:.c=.o)

# Target names
DESKTOP_TARGET = dungeonDesktop
PET_TARGET = dungeonPET.prg
C64_TARGET = dungeon64.prg

# Default target - build all platforms
all: desktop pet c64

# Desktop target using GCC
desktop: $(DESKTOP_OBJ)
	$(CC) $(CFLAGS) -o $(DESKTOP_TARGET) $^ $(LDFLAGS)

# PET target using CL65
pet: $(COMMON_SRC)
	$(CL65) -t pet -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o $(PET_TARGET) $(COMMON_SRC)

# C64 target using CL65
c64: $(COMMON_SRC)
	$(CL65) -t c64 -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o $(C64_TARGET) $(COMMON_SRC)

# Object file rule for desktop builds only
$(DESKTOP_OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(DESKTOP_OBJ) $(DESKTOP_TARGET) $(PET_TARGET) $(C64_TARGET)

.PHONY: all clean desktop pet c64 