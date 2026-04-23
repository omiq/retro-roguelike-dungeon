#!/bin/sh
cc dungeon.c -o ~/dungeon -lncurses
cl65 -t pet -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o dungeonPET.prg dungeon.c
cl65 -t c64 -v -o dungeon64.prg dungeon.c 
cl65 -t plus4 -v -Cl -O -DCC65_NO_RUNTIME_TYPE_CHECKS -o dungeonPlus4.prg dungeon.c
