/*
 * plat_plus4.c — Commodore Plus/4 adapter (cc65 + conio).
 * Plus/4 TED has 121 colors; cc65 conio exposes the 16 "standard" CBM slots.
 */
#include <conio.h>
#include <stdlib.h>
#include "platform.h"

static const uint8_t glyph_native[G_COUNT] = {
    '.','#','+','@','E','$','!','/','>','%', ' ','*', 'h','*','&', '*', 'k','-'
};

static const uint8_t colour_native[8] = {
    COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN,
    COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_VIOLET
};

void plat_init(void) {
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    clrscr();
}

void plat_shutdown(void) {
    textcolor(COLOR_WHITE);
    clrscr();
}

uint8_t plat_screen_w(void) { return 40; }
uint8_t plat_screen_h(void) { return 25; }

void plat_cls(void) { clrscr(); }

void plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour) {
    textcolor(colour_native[colour & 7]);
    cputcxy(x, y, glyph_native[g]);
}

void plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour) {
    textcolor(colour_native[colour & 7]);
    cputsxy(x, y, s);
}

uint8_t plat_key_pressed(void) {
    if (!kbhit()) return K_NONE;
    return plat_key_wait();
}

uint8_t plat_key_wait(void) {
    char c = cgetc();
    switch (c) {
        case 145: return K_UP;
        case 17:  return K_DOWN;
        case 157: return K_LEFT;
        case 29:  return K_RIGHT;
        case ' ': case 13:
        case 'z': case 'Z': return K_FIRE;
        case 'q': case 'Q':
        case 3:             return K_QUIT;
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
        for (i = 0; i < 180; i++) __asm__("nop");
    }
}

static uint16_t rng_state;
void plat_seed_rand(uint16_t seed) {
    if (seed) { rng_state = seed; return; }
    /* TED raster low byte ($FF1D) + timer 1 low ($FF00). */
    rng_state = (uint16_t)(*(volatile uint8_t *)0xFF1D) |
                ((uint16_t)(*(volatile uint8_t *)0xFF00) << 8);
    if (!rng_state) rng_state = 0xbeef;
}
uint16_t plat_rand(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

void plat_beep(uint16_t freq_hz, uint8_t dur_ms) {
    (void)freq_hz; (void)dur_ms;
}
