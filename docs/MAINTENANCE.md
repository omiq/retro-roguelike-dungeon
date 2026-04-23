# retro-c Maintenance Guide

Quick reference for working on this codebase without re-breaking portability.
Read this first every time you come back to the project.

## Repo layout (recap)

```
game/            portable C89 game logic — DO include platform.h + other game headers only
  main.c map.c entity.c
  glyphs.h map.h entity.h
platform/        one plat_<target>.c per supported machine
  platform.h     API contract — all game code goes through this
  plat_c64.c plat_pet.c plat_plus4.c plat_vic20.c
  plat_apple2.c plat_atari8.c plat_bbc.c plat_host.c
build/
  common.mk      shared GAME_SOURCES list
  Makefile.<plat>
ARCHIVE_pre_refactor/   frozen pre-refactor snapshot. Do not edit.
docs/
  refactor-plan.md   full plan + progress log (append to it, don't rewrite)
  MAINTENANCE.md     this file
```

## Golden rules

1. **`game/*.c` never includes anything from `<conio.h>`, `cbm.h`, POKE/PEEK
   macros, `__C64__`/`__PET__`/etc. ifdefs, or platform-specific headers.**
   Only `<stdint.h>`, other `game/*.h`, `platform/platform.h`.
2. **No `40`/`25`/`22` literals in game code.** Use `plat_screen_w()` /
   `plat_screen_h()`. If you hard-code dims you'll break VIC-20 / GB later.
3. **No raw screencodes / PETSCII / ATASCII in game code.** Use `G_*` glyph
   enums; adapters own the translation table.
4. **No raw palette indices.** Use `COL_*` (WHITE/RED/GREEN/BLUE/YELLOW/
   CYAN/MAGENTA/BLACK). Adapter maps to native palette slot.
5. **Keys are `K_UP` / `K_DOWN` / `K_LEFT` / `K_RIGHT` / `K_FIRE` / `K_QUIT`
   / `K_OTHER`** — never raw scancodes or PETSCII chars in game code.
6. **Incremental redraw.** Don't repaint the full screen each turn. Touch
   only cells that changed (player old/new, enemies that moved, status
   bar when stats differ). See `game/main.c` patterns.

## Adding a feature to the game (common case)

All game features live in `game/`. Most additions require **zero adapter
changes**:

- New glyph type (e.g. `G_TRAP`, `G_SCROLL`): add to `game/glyphs.h`, update
  `G_COUNT`, add entry in every `plat_*.c`'s `glyph_native[]` table, add
  colour mapping in `game/main.c` `colour_for_glyph()`.
- New enemy behaviour: edit `game/entity.c`. `entity_ai_turn()` is the
  single place AI runs.
- New item pickup: edit `step_onto()` in `game/main.c`.
- New map room: add string array in `game/map.c` and bump `ROOM_COUNT`.
- Sound: add `plat_beep(freq, dur)` calls in game logic — adapters no-op
  on platforms that don't implement it yet.

**If you need something the adapter API can't express** — e.g. drawing
a multi-cell sprite, scrolling the viewport, reading analog joystick —
first try to express it as one or more existing calls. Only extend
`platform.h` as a last resort. Any new function MUST be implemented in
every `plat_*.c` (even as a no-op stub) or builds break.

## Building

Host (fastest iteration — runs on desktop with ncurses):
```
make -f build/Makefile.host run
```

Individual platforms:
```
make -f build/Makefile.c64
make -f build/Makefile.pet
make -f build/Makefile.plus4
make -f build/Makefile.apple2
make -f build/Makefile.atari8
make -f build/Makefile.bbc
make -f build/Makefile.vic20   # needs +32K cart; xvic -memory all
```

All at once:
```
for t in c64 pet plus4 vic20 apple2 atari8 bbc host; do
  make -f build/Makefile.$t clean
  make -f build/Makefile.$t || echo "FAIL: $t"
done
```

If one platform breaks but others compile, you leaked a platform
assumption into `game/`. Fix there, not in the adapter.

## Test checklist before committing

Run **all** of these on every non-trivial game logic change:

1. `make -f build/Makefile.host run` — walk around, pick up gold, bump an
   enemy, die. HP/GOLD/DMG numbers update correctly.
2. At least one 8-bit target builds (c64 is fastest feedback loop):
   `make -f build/Makefile.c64`. Load `dungeon-c64.prg` in VICE.
3. Memory-tight target: `make -f build/Makefile.vic20`. If overflow
   error, game is too big for unexpanded cfg — don't lower targets
   silently, add a tier flag instead.

Commit message format: one-line summary + bullet list of changes
(see existing commits). Reference phase numbers from `refactor-plan.md`.

## Updating the 8bitworkshop IDE demo presets

The IDE presets live in a *separate* repo: `/Users/chrisg/github/8bitworkshop`.
The single-file demos at `presets/<plat>/retroc-dungeon.c` are **manually
maintained snapshots** of this repo's game+adapter combined.

Whenever you change `game/*.c` or `platform/plat_<plat>.c`, regenerate
the corresponding IDE preset:

1. Concatenate inline: game enum/struct defs → adapter → map → entity →
   main. No `#include` directives for game headers — paste contents.
2. Copy to `8bitworkshop/presets/<plat>/retroc-dungeon.c`.
3. Test build in the IDE: `?platform=<plat>&file=retroc-dungeon.c` →
   Build. Confirm no cc65 errors.
4. Deploy: `cd ~/github/8bitworkshop && ./deploy.sh`.

When Phase 5 or later adds a code generator script (`scripts/bundle-ide-demo.py`),
skip the manual concat step.

## Adding a new platform

Checklist for a new `plat_<target>.c`:

1. **API conformance** — implement *every* function in `platform.h`.
   `plat_beep` can no-op. `plat_key_pressed` can return `K_NONE`.
2. **Glyph table** — `glyph_native[G_COUNT]` — pick readable native chars.
   Tile-based targets (GB/NES) upload a tileset at `plat_init` and store
   tile indices here.
3. **Colour table** — `colour_native[8]` for `COL_*`. Use nearest native
   palette slot. Mono targets ignore the colour arg.
4. **Keys** — translate native scancode/PETSCII/ATASCII to `K_*`.
   Always support both arrow keys AND WASD fallback (some emulators
   quirk cursor keys).
5. **Screen dims** — return actual visible char grid size (NOT pixel
   resolution). For tile targets use tile-grid dims, not pixel.
6. **`Makefile.<target>`** in `build/` — include `common.mk`, list
   `$(GAME_SOURCES) platform/plat_<target>.c`, set `cl65 -t <target>`
   (or appropriate compiler).
7. **Add to test-all loop** in this file's "Building" section.
8. **Update `refactor-plan.md`** progress log.

Memory-tight targets (VIC-20 unexpanded 3.5K, GB 32K ROM) may need a
tier flag in `game/config.h` — gate enemy/item tables by
`#ifdef TIER_MINIMAL`.

## Debugging platform breakage

Symptom → likely cause:

- `Segment 'CODE' overflows MAIN by N bytes` → memory tier too small.
  Either use expanded cfg (e.g. `vic20-32k.cfg`) or add `TIER_MINIMAL`
  gates to shrink tables.
- `Unresolved external 'gotoxy'` (BBC) → cc65 port missing symbol.
  Roll your own `plat_puts` via `cputcxy` loop (already done in
  `plat_bbc.c`).
- `Undefined COLOR_PURPLE` → cc65 platform uses `COLOR_VIOLET` instead
  (VIC-20, Plus/4). Fix adapter colour_native table.
- `'/*' within block comment` warning → avoid `/*` inside block comments.
- Game crashes on one target but works elsewhere → you put platform-
  specific code in `game/`. Move it behind `platform.h`.

## When in doubt

Run host (ncurses) first — fastest feedback. If host works and a target
breaks, the bug is in that specific `plat_*.c`. If host also misbehaves,
the bug is in `game/`.

Keep the golden rules. Adapter API stays tight. Portability compounds.
