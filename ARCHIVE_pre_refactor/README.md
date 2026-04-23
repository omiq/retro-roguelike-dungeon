# ARCHIVE — Pre-Refactor Snapshot

Everything in this directory is the **frozen** state of retro-c before the
cross-platform refactor (branch `refactor/cross-platform-skeleton`, 2026-04-23).

**Do not edit anything under here.** It exists only as a reference and as a
source of code that will be migrated into the new layout:

```
../game/       platform-neutral game logic (C89)
../platform/   one plat_<target>.c per platform
../build/      per-platform Makefiles
```

## What is here

- `dungeon.c`, `dungeon_multi.c` — previous main game sources with inline
  `#ifdef __C64__` / `#ifdef __PET__` blocks. These are the primary source
  of game logic being split into `game/` + `platform/` modules.
- `map.h`, `maze.h`, `maze.c` — dungeon / maze generators, to port into
  `game/map.c`.
- `charset-c64.c`, `chars.bin`, `charset.cst` — custom C64 charset, stays
  as data asset, referenced by `platform/plat_c64.c` init.
- `ansi.h`, `notconio.h`, `simpleio.h`, `load2.h` — host-side stubs,
  candidates for absorption into `platform/plat_host.c`.
- `cpm-*.c` — CP/M target experiments. Fold into `platform/plat_cpm.c`
  only if CP/M stays as a supported platform; otherwise delete at end of
  migration.
- `welcome.c`, `wordy.c` — older title / demo programs.
- `demo/`, `rasterscroll/`, `src/`, `python/`, `welcome/` — older experiments.
- `*.prg`, `*.d64`, `*.com`, `*.dsk`, `*.png`, build scripts — binary
  artifacts and screenshots from the pre-refactor builds. Kept so we can
  diff against new builds.

## Removal criteria

Once a file's logic has been migrated into `game/` or `platform/` AND the
new version has shipped on the same target with behavioural parity, the
archived copy can be deleted in a cleanup commit. Track progress in
`docs/refactor-plan.md` checklist (to be added).
