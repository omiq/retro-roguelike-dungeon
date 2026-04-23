# retro-c Roguelike Refactor Plan

Goal: take existing `dungeon_multi.c` (cc65, ~1265 LoC, mixed CBM ifdefs) and split into
platform-agnostic game/ + thin platform/plat_*.c adapters so one codebase ships to:
c64, pet, plus4, vic20, atari8, apple2, bbc, nes, gb, plus host (ncurses) for dev.

## Target layout

```
retro-c/
  game/                 # platform-neutral C89, no includes beyond <stdint.h> + platform.h
    main.c              # event loop: init -> title -> play -> gameover
    player.c  .h
    enemies.c .h
    map.c     .h        # dungeon gen, FOV, map[] access
    items.c   .h
    combat.c  .h
    glyphs.h            # G_WALL, G_FLOOR, G_PLAYER, G_ENEMY, G_GOLD... symbolic
  platform/
    platform.h          # the contract (below)
    glyph_map.h         # each plat includes this; maps G_* -> native screencode/tile
    plat_c64.c
    plat_pet.c
    plat_vic20.c
    plat_plus4.c
    plat_apple2.c
    plat_atari8.c
    plat_bbc.c
    plat_gb.c           # sdcc/GBDK — different build chain
    plat_nes.c          # cc65-nes, needs nametable abstraction
    plat_host.c         # ncurses for desktop dev (fast iteration)
  build/
    common.mk           # game/*.c list, flags
    Makefile.c64  ...   # one per platform, include common.mk
  assets/
    charset-c64.bin     # existing custom chars; one per platform that needs it
    tileset-gb.bin      # GB 8×8 tiles for G_*
  legacy/               # park existing dungeon.c / dungeon_multi.c / cpm-game.c
                        # until their logic is fully migrated; delete only after parity.
```

## Platform API (platform/platform.h)

Minimal surface, every adapter must implement. Freeze this first — new calls
added reluctantly. ~15 functions.

```c
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include "glyphs.h"

/* Lifecycle */
void    plat_init(void);
void    plat_shutdown(void);

/* Screen dimensions — compile-time per platform */
#define PLAT_W  plat_screen_w()
#define PLAT_H  plat_screen_h()
uint8_t plat_screen_w(void);
uint8_t plat_screen_h(void);

/* Drawing */
void    plat_cls(void);
void    plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t colour);
void    plat_puts(uint8_t x, uint8_t y, const char *s, uint8_t colour);

/* Input — non-blocking + blocking */
uint8_t plat_key_pressed(void);    /* 0 = none; else scancode-ish */
uint8_t plat_key_wait(void);       /* block until key */

/* Timing */
void    plat_delay_ms(uint16_t ms);

/* RNG — some platforms have cheap hw entropy (raster line, jiffy clock) */
uint16_t plat_rand(void);
void     plat_seed_rand(uint16_t seed);

/* Sound — optional, may no-op on minimal builds */
void    plat_beep(uint16_t freq_hz, uint8_t dur_ms);

#endif
```

## Glyph abstraction (platform/glyphs.h)

```c
typedef uint8_t glyph_t;
#define G_FLOOR   0
#define G_WALL    1
#define G_DOOR    2
#define G_PLAYER  3
#define G_ENEMY   4
#define G_GOLD    5
#define G_POTION  6
#define G_WEAPON  7
#define G_STAIRS  8
/* ...up to 32; game never uses raw chars */
#define G_COUNT   16
```

Each plat_*.c holds a native lookup:
```c
/* plat_c64.c */
static const uint8_t glyph_native[G_COUNT] = {
  [G_FLOOR]=46, [G_WALL]=35, [G_PLAYER]=0, [G_ENEMY]=81, [G_GOLD]=36, ...
};
void plat_putc(uint8_t x, uint8_t y, glyph_t g, uint8_t col) {
    cputcxy(x,y,glyph_native[g]);
    /* cc65 handles color via textcolor() — call before cputcxy for perf */
}
```

GB / NES use the same table but indices point to tile#s uploaded at init.

## Dimension contract

Game code must not assume 40×25. Pass dimensions via `PLAT_W`/`PLAT_H`. Map
chunk size falls out of platform header:

```
c64, pet, plus4, atari8, bbc, apple2 -> 40×25 (or 40×24)
vic20                                -> 22×23
gb                                   -> 20×18 (BG tile grid)
nes                                  -> 32×30
```

Map generator picks chunk dims from `PLAT_W`/`PLAT_H` at init. Smaller screens
just show scroll window.

## Memory budget / data tiers

Some targets tight. Use feature flags:

```c
/* game/config.h picks tier from platform __DEFINE__ */
#if defined(__GAMEBOY__) || defined(__VIC20__)
  #define TIER_MINIMAL  1    /* fewer enemy types, smaller tables */
#elif defined(__NES__)
  #define TIER_STANDARD 1
#else
  #define TIER_FULL     1    /* full monster manual, all items */
#endif
```

Monster tables gated by tier — game logic reads `enemy_types[]` + `n_enemy_types`,
both set at compile time per tier.

## Build tooling

Each `build/Makefile.<plat>` picks tools:

```
# build/Makefile.c64
TARGET   = c64
CC       = cl65
CFLAGS   = -t c64 -O -Cl
OBJS     = game/main.o game/player.o ... platform/plat_c64.o
dungeon-c64.prg: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
```

`build/common.mk` lists game/*.c so new game modules drop in once.

GB Makefile uses `lcc` or `sdcc + makebin-gb`; NES uses `cl65 -t nes`. Host uses
`gcc game/*.c platform/plat_host.c -lncurses -o dungeon`.

## Migration path (phased, each phase ends with c64 still runnable)

**Phase 0 — safety net (1 day)**
- Copy `dungeon_multi.c` untouched into `legacy/`. Keep PRGs in repo.
- Add `build/Makefile.legacy` that builds the old file on c64/pet. Baseline.

**Phase 1 — adapter skeleton (1 day)**
- Write `platform/platform.h` + `plat_c64.c` wrapping cputcxy/cgetc/etc.
- Write `game/glyphs.h` + stub `game/main.c` that draws @ in center + reads key.
- Build c64 PRG. Smoke test: pet also builds with `plat_pet.c` (copy of c64, swap screen_w/h).

**Phase 2 — pull map gen out (2 days)**
- Move room/corridor gen from dungeon_multi into `game/map.c`, keep logic identical.
- Replace direct POKE/cputcxy on map cells with `plat_putc(x,y,G_WALL,col)`.
- Game builds + runs on c64 with empty player/enemy modules.

**Phase 3 — entities (2 days)**
- Extract player/enemy/item structs + update loop into `game/*.c`.
- All drawing via plat_putc; input via plat_key_wait/plat_key_pressed.
- c64 playable again. pet + plus4 now build for free (adapters already done).

**Phase 4 — widen target set (1 day each, parallelisable)**
- Add `plat_apple2.c`, `plat_atari8.c`, `plat_bbc.c`, `plat_vic20.c`.
- Each = ~50 LoC adapter + glyph table tuned to that charset.
- Host `plat_host.c` via ncurses for desktop dev — fastest to iterate on game logic.

**Phase 5 — GB + NES (2-3 days each)**
- GB: plat_gb.c uploads a tileset at init, uses `set_bkg_tile_xy`. No conio.
- NES: similar tile-based model via cc65 nes lib. Needs vblank sync.
- These confirm the API is actually tight enough. If API leaks 40×25 assumptions,
  this is where you'll find out.

**Phase 6 — delete legacy/ (1 day)**
- Once every platform ships from game/ + platform/, drop legacy/ and old .c files.

Total: ~2 weeks focused, or stretch out module-by-module.

## What survives from existing code

Salvage opportunistically, don't rewrite from scratch:

- `map.h` dungeon generator — port to `game/map.c` directly, minor cleanup.
- `maze.h` — same.
- `charset-c64.c` — stays as data, referenced by `plat_c64.c` init.
- `notconio.h` / `simpleio.h` / `ansi.h` — likely absorbed into `plat_host.c`.
- `cpm-*.c` — fold into `plat_cpm.c` if CP/M stays a target, else archive.
- `welcome.c`, `wordy.c` — merge into `game/title.c` if they drive the intro.

## Risks

- **cc65 NES target** screen model is tile+attribute, not char+color. Adapter
  will need a small attribute-table cache to avoid per-putc attribute rewrites.
- **GB** playable area is 20×18 tiles visible — roguelike map > viewport, need
  scroll camera. Either scroll BG registers or redraw each move. Latter simpler.
- **VIC-20** RAM — 3.5 KB free unexpanded. Tier=MINIMAL must actually fit. If
  not, require 8K/16K cart config in Makefile.
- **Apple II** HGR vs text modes. Stay in 40-col text mode for roguelike, no HGR.
- **Color** varies widely. Make `plat_putc` colour arg a small enum (`COL_WHITE`,
  `COL_RED`, ...) not a raw palette index.

## First step

Clone retro-c, make `legacy/` dir, move existing .c files in, commit. Then land
`platform/platform.h` + `plat_c64.c` + `game/main.c` with just "draw @, read key,
move @". Builds c64 PRG via `make -f build/Makefile.c64`. That's Phase 0+1
baseline and gives a clean floor to migrate onto.
