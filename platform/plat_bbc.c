/*
 * plat_bbc.c — BBC Micro adapter (cc65 + conio, partial).
 * BBC cc65 port lacks cputsxy/gotoxy symbols in this toolchain, so
 * plat_puts rolls its own via cputcxy. Mono drawing.
 */
#include <conio.h>
#include <stdlib.h>
#include "platform.h"

static const uint8_t glyph_native[G_COUNT] = {
    '.','#','+','@','E','$','!','/','>','%', ' ','*', 'h','m','&', '*', 'k'
};

void plat_init(void) { clrscr(); }
void plat_shutdown(void) { clrscr(); }

uint8_t plat_screen_w(void) { return 40; }
uint8_t plat_screen_h(void) { return 25; }

void plat_cls(void) { clrscr(); }

void plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour) {
    (void)colour;
    cputcxy(x, y, glyph_native[g]);
}

void plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour) {
    uint8_t i = 0;
    (void)colour;
    while (s[i]) { cputcxy(x + i, y, s[i]); i++; }
}

uint8_t plat_key_pressed(void) {
    if (!kbhit()) return K_NONE;
    return plat_key_wait();
}

uint8_t plat_key_wait(void) {
    char c = cgetc();
    switch (c) {
        case 8:   return K_LEFT;
        case 9:   return K_RIGHT;
        case 11:  return K_UP;
        case 10:  return K_DOWN;
        case ' ': case 13:
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
        for (i = 0; i < 200; i++) __asm__("nop");
    }
}

static uint16_t rng_state;
void plat_seed_rand(uint16_t seed) {
    if (seed) { rng_state = seed; return; }
    /* BBC OS has no trivial zero-page timer; use system VIA timer T1
     * ($FE44/FE45 for System VIA). Advances 1MHz, so 2 bytes ≈ random. */
    rng_state = (uint16_t)(*(volatile uint8_t *)0xFE44) |
                ((uint16_t)(*(volatile uint8_t *)0xFE45) << 8);
    if (!rng_state) rng_state = 0xbeef;
}
uint16_t plat_rand(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

void plat_beep(uint16_t freq_hz, uint8_t dur_ms) { (void)freq_hz; (void)dur_ms; }
