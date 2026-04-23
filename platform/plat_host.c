/*
 * plat_host.c — desktop (ncurses) adapter for fast iteration on game logic.
 * Not a shipping target; builds on Linux/macOS via:
 *   gcc -DPLAT_HOST=1 -Iplatform -Igame game/main.c game/map.c \
 *       game/entity.c platform/plat_host.c -lncurses -o dungeon-host
 */
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "platform.h"

static uint8_t screen_cols;
static uint8_t screen_rows;

static const char glyph_native[G_COUNT] = {
    '.','#','+','@','E','$','!','/','>','%', ' ','*', 'h','m','&', '*', 'k','-'
};

/* ncurses colour pairs 1..8 map to COL_* */
static const short fg_for_col[8] = {
    COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN,
    COLOR_BLUE,  COLOR_YELLOW, COLOR_CYAN, COLOR_MAGENTA
};

void plat_init(void) {
    int rows, cols, i;
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors()) {
        start_color();
        use_default_colors();
        for (i = 0; i < 8; i++) init_pair(i + 1, fg_for_col[i], COLOR_BLACK);
    }
    getmaxyx(stdscr, rows, cols);
    if (cols > 80) cols = 80;
    if (rows > 25) rows = 25;
    screen_cols = (uint8_t)cols;
    screen_rows = (uint8_t)rows;
    erase();
    refresh();
}

void plat_shutdown(void) { endwin(); }

uint8_t plat_screen_w(void) { return screen_cols ? screen_cols : 40; }
uint8_t plat_screen_h(void) { return screen_rows ? screen_rows : 25; }

void plat_cls(void) { erase(); refresh(); }

void plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour) {
    attron(COLOR_PAIR((colour & 7) + 1));
    mvaddch(y, x, glyph_native[g]);
    attroff(COLOR_PAIR((colour & 7) + 1));
    refresh();
}

void plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour) {
    attron(COLOR_PAIR((colour & 7) + 1));
    mvaddstr(y, x, s);
    attroff(COLOR_PAIR((colour & 7) + 1));
    refresh();
}

uint8_t plat_key_pressed(void) {
    int c;
    nodelay(stdscr, TRUE);
    c = getch();
    nodelay(stdscr, FALSE);
    if (c == ERR) return K_NONE;
    ungetch(c);
    return plat_key_wait();
}

uint8_t plat_key_wait(void) {
    int c = getch();
    switch (c) {
        case KEY_UP:    return K_UP;
        case KEY_DOWN:  return K_DOWN;
        case KEY_LEFT:  return K_LEFT;
        case KEY_RIGHT: return K_RIGHT;
        case ' ': case '\n':
        case 'z': case 'Z': return K_FIRE;
        case 'q': case 'Q': case 27: return K_QUIT;
        case 'w': case 'W': return K_UP;
        case 's': case 'S': return K_DOWN;
        case 'a': case 'A': return K_LEFT;
        case 'd': case 'D': return K_RIGHT;
        default:            return K_OTHER;
    }
}

void plat_delay_ms(uint16_t ms) { usleep((useconds_t)ms * 1000); }

static uint16_t rng_state;
void plat_seed_rand(uint16_t seed) {
    if (seed) rng_state = seed;
    else      rng_state = (uint16_t)time(NULL);
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
    beep();
}
