/*
 * plat_c64.c — Commodore 64 adapter (cc65 + conio).
 * Logical glyphs mapped to C64 PETSCII/screencode chars.
 * Logical colours mapped to native C64 palette slots.
 */
#include <conio.h>
#include <stdlib.h>
#include "platform.h"

static const uint8_t glyph_native[G_COUNT] = {
    /* G_FLOOR  */ '.',
    /* G_WALL   */ '#',
    /* G_DOOR   */ '+',
    /* G_PLAYER */ '@',
    /* G_ENEMY  */ 'E',
    /* G_GOLD   */ '$',
    /* G_POTION */ '!',
    /* G_WEAPON */ '/',
    /* G_STAIRS */ '>',
    /* G_CORPSE */ '%',
    /* G_SPACE  */ ' ',
    /* G_BORDER */ '*',
    /* G_HEALTH */ 'h',
    /* G_MAGIC  */ 'm',
    /* G_IDOL   */ '&',
    /* G_BOLT   */ '*',
    /* G_KEY    */ 'k',
};

static const uint8_t colour_native[8] = {
    COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN,
    COLOR_BLUE, COLOR_YELLOW, COLOR_CYAN, COLOR_PURPLE
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
    /* Map PETSCII to logical keys. C64 cursor keys send $91/$11/$9D/$1D. */
    char c = cgetc();
    switch (c) {
        case 145: /* CRSR UP     */ return K_UP;
        case 17:  /* CRSR DOWN   */ return K_DOWN;
        case 157: /* CRSR LEFT   */ return K_LEFT;
        case 29:  /* CRSR RIGHT  */ return K_RIGHT;
        case ' ':
        case 13:
        case 'z': case 'Z':         return K_FIRE;
        case 'q': case 'Q':
        case 3:   /* STOP / CTRL-C (BREAK) */ return K_QUIT;
        case 'w': case 'W':         return K_UP;
        case 's': case 'S':         return K_DOWN;
        case 'a': case 'A':         return K_LEFT;
        case 'd': case 'D':         return K_RIGHT;
        default:                    return K_OTHER;
    }
}

void plat_delay_ms(uint16_t ms) {
    /* cc65 has no sleep(); crude busy-wait (PAL ~1 MHz). */
    while (ms--) {
        uint16_t i;
        for (i = 0; i < 180; i++) __asm__("nop");
    }
}

/* Seed from VIC raster line ($D012) — cheap entropy at boot. */
static uint16_t rng_state;

void plat_seed_rand(uint16_t seed) {
    if (seed) { rng_state = seed; return; }
    /* 0 = pull entropy from VIC-II raster ($D012) + TOD tenths ($DC08).
     * Player's boot time + idle time before first key mix the values. */
    rng_state = (uint16_t)(*(volatile uint8_t *)0xD012) |
                ((uint16_t)(*(volatile uint8_t *)0xDC08) << 8);
    if (!rng_state) rng_state = 0xbeef;
}

uint16_t plat_rand(void) {
    /* xorshift16 */
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

void plat_beep(uint16_t freq_hz, uint8_t dur_ms) {
    (void)freq_hz; (void)dur_ms;
    /* TODO: SID voice 1 triangle wave. For now no-op keeps skeleton tiny. */
}
