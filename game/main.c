/*
 * main.c — smoke-test game loop.
 * Draws '@' in middle of screen, arrow keys move, Q quits.
 * No map, no enemies yet. Just proves the adapter API works end-to-end
 * for each platform before real game modules migrate in.
 */
#include <stdint.h>
#include "../platform/platform.h"

static uint8_t px, py;

static void redraw_player(uint8_t old_x, uint8_t old_y) {
    plat_putc(old_x, old_y, G_FLOOR, COL_WHITE);
    plat_putc(px,    py,    G_PLAYER, COL_YELLOW);
}

int main(void) {
    uint8_t w, h, key, nx, ny;

    plat_init();
    plat_cls();

    w = plat_screen_w();
    h = plat_screen_h();
    px = w / 2;
    py = h / 2;

    plat_puts(0, 0, "retro-c dungeon skeleton", COL_CYAN);
    plat_puts(0, 1, "arrows move, q quit",       COL_WHITE);
    plat_putc(px, py, G_PLAYER, COL_YELLOW);

    for (;;) {
        key = plat_key_wait();
        nx = px;
        ny = py;
        switch (key) {
            case K_UP:    if (py > 2)     ny = py - 1; break;
            case K_DOWN:  if (py < h - 1) ny = py + 1; break;
            case K_LEFT:  if (px > 0)     nx = px - 1; break;
            case K_RIGHT: if (px < w - 1) nx = px + 1; break;
            case K_QUIT:  plat_shutdown(); return 0;
            default: continue;
        }
        if (nx != px || ny != py) {
            uint8_t old_x = px, old_y = py;
            px = nx; py = ny;
            redraw_player(old_x, old_y);
        }
    }
}
