/*
 * plat_atari8.c — Atari 8-bit (400/800/XL/XE) adapter (cc65 + conio).
 * Default ANTIC mode 0 text: 40×24.
 */
#include <conio.h>
#include <stdlib.h>
#include "platform.h"

static const uint8_t glyph_native[G_COUNT] = {
    '.','#','+','@','E','$','!','/','>','%', ' ','*', 'h','*','&', '*', 'k','-'
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

/* Atari 8-bit: cc65 cgetc returns ATASCII. Cursor keys are ctrl+arrow. */
uint8_t plat_key_wait(void) {
    char c = cgetc();
    switch (c) {
        case 28:  return K_UP;     /* ctrl- - */
        case 29:  return K_DOWN;
        case 30:  return K_LEFT;
        case 31:  return K_RIGHT;
        case ' ': case 155: /* RETURN/EOL */
        case 'z': case 'Z': return K_FIRE;
        case 'q': case 'Q': case 27: return K_QUIT;
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
        for (i = 0; i < 160; i++) __asm__("nop");
    }
}

static uint16_t rng_state;
void plat_seed_rand(uint16_t seed) {
    if (seed) { rng_state = seed; return; }
    /* Atari RTCLOK jiffy $12..$14 + RANDOM $D20A (POKEY). */
    rng_state = (uint16_t)(*(volatile uint8_t *)0xD20A) |
                ((uint16_t)(*(volatile uint8_t *)0x0014) << 8);
    if (!rng_state) rng_state = 0xbeef;
}
uint16_t plat_rand(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

void plat_beep(uint16_t freq_hz, uint8_t dur_ms) { (void)freq_hz; (void)dur_ms; }
