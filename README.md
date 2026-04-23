# Retro Roguelike Dungeon

Cross-platform retro roguelike in C89. One codebase, many 8-bit targets.

**Status:** Phase 0+1 refactor (2026-04-23). Previous monolithic sources are
frozen under `ARCHIVE_pre_refactor/` and being migrated module by module.

## Layout

```
game/           platform-neutral game logic (C89, no conio, no __C64__ ifdefs)
  main.c        smoke test: draw @, arrow keys move
  glyphs.h      symbolic G_* glyphs — adapters map to native chars/tiles
platform/       one adapter per target
  platform.h    API contract (~15 funcs: init, cls, putc, puts, key_wait,
                delay, rand, beep, screen_w/h) + K_* key codes + COL_* colours
  plat_c64.c    Commodore 64 (cc65 + conio)
build/          one Makefile per target
  common.mk     shared GAME_SOURCES list
  Makefile.c64  cl65 -t c64
ARCHIVE_pre_refactor/
                frozen pre-refactor repo; do not edit; source of migration.
docs/           plan + notes
```

## Build

### C64

```
make -f build/Makefile.c64
```

Output: `dungeon-c64.prg`. Run in VICE: `make -f build/Makefile.c64 run`.

Other platforms land as adapters are written — see roadmap.

## Roadmap

- [x] Phase 0: archive old sources, branch `refactor/cross-platform-skeleton`
- [x] Phase 1: platform.h contract + plat_c64.c + game/main.c smoke test
- [ ] Phase 2: migrate `map.h` generator to `game/map.c`
- [ ] Phase 3: migrate player/enemies/items from `ARCHIVE_pre_refactor/dungeon_multi.c`
- [ ] Phase 4: widen — `plat_pet.c`, `plat_plus4.c`, `plat_vic20.c`,
  `plat_apple2.c`, `plat_atari8.c`, `plat_bbc.c`, `plat_host.c` (ncurses)
- [ ] Phase 5: tile-based targets — `plat_gb.c` (GBDK), `plat_nes.c`
- [ ] Phase 6: delete `ARCHIVE_pre_refactor/`

## Rules for new code

- `game/*.c` includes only `<stdint.h>`, other `game/*.h`, and `platform/platform.h`.
- `game/*.c` never calls `conio.h`, `cbm.h`, POKE/PEEK, `cputcxy`.
- Native drawing goes through `plat_putc` / `plat_puts`.
- Native input goes through `plat_key_wait` / `plat_key_pressed` returning `K_*`.
- Screen dimensions via `plat_screen_w()` / `plat_screen_h()`, no `40`/`25` literals.
- Colour arg is a `COL_*` logical enum, not a raw palette index.
- Each new adapter just implements `platform.h`. Game code unchanged.
