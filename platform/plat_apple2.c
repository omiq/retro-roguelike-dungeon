/*
 * plat_apple2.c — Apple ][ / ][+ / //e 40-column text adapter (cc65 + conio).
 * Apple II text is 40×24 with 2-colour attributes per line (no per-cell colour).
 * cc65 conio textcolor() still accepts COLOR_* but is effectively monochrome.
 */
#include <conio.h>
#include <stdlib.h>
#include "platform.h"

static const uint8_t glyph_native[G_COUNT] = {
    '.','#','+','@','E','$','!','/','>','%', ' ','*', 'h','m','&'
};

void plat_init(void) { clrscr(); }
void plat_shutdown(void) { clrscr(); }

uint8_t plat_screen_w(void) { return 40; }
uint8_t plat_screen_h(void) { return 24; }

void plat_cls(void) { clrscr(); }

void plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour) {
    (void)colour;
    cputcxy(x, y, glyph_native[g]);
}

void plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour) {
    (void)colour;
    cputsxy(x, y, s);
}

uint8_t plat_key_pressed(void) {
    if (!kbhit()) return K_NONE;
    return plat_key_wait();
}

/* Apple II cgetc returns ASCII. Arrows = 8/10/11/21 typically, but many
 * monitors translate; fall back on WASD. */
uint8_t plat_key_wait(void) {
    char c = cgetc();
    switch (c) {
        case 8:   return K_LEFT;   /* ctrl-H */
        case 21:  return K_RIGHT;  /* ctrl-U */
        case 11:  return K_UP;     /* ctrl-K */
        case 10:  return K_DOWN;   /* ctrl-J */
        case ' ': case 13:
        case 'z': case 'Z': return K_FIRE;
        case 'q': case 'Q': case 3: return K_QUIT;
        case 'w': case 'W': return K_UP;
        case 's': case 'S': return K_DOWN;
        case 'a': case 'A': return K_LEFT;
        case 'd': case 'D': return K_RIGHT;
        default:            return K_OTHER;
    }
}

void plat_delay_ms(uint16_t ms) {
    while (ms--) {
        uint16_t i;
        for (i = 0; i < 100; i++) __asm__("nop");   /* ~1 MHz */
    }
}

static uint16_t rng_state;
void plat_seed_rand(uint16_t seed) { rng_state = seed ? seed : 0xbeef; }
uint16_t plat_rand(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

void plat_beep(uint16_t freq_hz, uint8_t dur_ms) { (void)freq_hz; (void)dur_ms; }
