#ifndef INPUT_H
#define INPUT_H

#include "../include/globals.h"

// Input functions
unsigned char get_key(void);
unsigned int dumb_wait(unsigned int delay);

#if defined (__CC65__)
// CC65 specific functions
void echo(void);
void refresh(void);
#else
// Non-CC65 functions
int kbhit(void);
#endif

#endif /* INPUT_H */ 