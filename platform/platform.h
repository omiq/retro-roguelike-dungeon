/*
 * platform.h — portable adapter API for retro-c dungeon.
 * One plat_<target>.c per platform implements this.
 * game source files must not include conio.h, POKE macros, __C64__
 * ifdefs, or platform-specific screen widths. Go through this header.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "../game/glyphs.h"

/* ---- Lifecycle ---- */
void    plat_init(void);
void    plat_shutdown(void);

/* ---- Screen ---- */
uint8_t plat_screen_w(void);
uint8_t plat_screen_h(void);
void    plat_cls(void);
void    plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour);
void    plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour);

/* ---- Input ---- */
uint8_t plat_key_pressed(void);   /* 0 = none; else logical key code (K_*) */
uint8_t plat_key_wait(void);      /* block until key */

/* Logical key codes — adapters must translate native scan/PETSCII to these */
#define K_NONE   0
#define K_UP     1
#define K_DOWN   2
#define K_LEFT   3
#define K_RIGHT  4
#define K_FIRE   5   /* space / return / z */
#define K_QUIT   6   /* q / esc */
#define K_OTHER  255

/* ---- Timing ---- */
void     plat_delay_ms(uint16_t ms);

/* ---- RNG ---- */
uint16_t plat_rand(void);
void     plat_seed_rand(uint16_t seed);

/* ---- Sound (optional; no-op on minimal builds) ---- */
void    plat_beep(uint16_t freq_hz, uint8_t dur_ms);

/* ---- Colours — logical, not palette index.
   Adapter maps to nearest native palette slot. ---- */
#define COL_BLACK   0
#define COL_WHITE   1
#define COL_RED     2
#define COL_GREEN   3
#define COL_BLUE    4
#define COL_YELLOW  5
#define COL_CYAN    6
#define COL_MAGENTA 7

#endif /* PLATFORM_H */
