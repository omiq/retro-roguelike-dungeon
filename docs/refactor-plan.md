# Roguelike Refactor Plan

Goal: take existing `dungeon_multi.c` (cc65, ~1265 LoC, mixed CBM ifdefs) and split into
platform-agnostic game/ + thin platform/plat_*.c adapters so one codebase ships to:
c64, pet, plus4, expanded vic20?, atari 800XL, apple2e, bbc micro, nes, gb, plus desktop (ncurses) for dev.

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

---

## PROGRESS LOG

Entries append-only. Most recent at bottom. Each entry = date, phase,
bullet list of concrete changes, one "why" line where non-obvious.

### 2026-04-23 — Phase 0+1 (commit `4e40110`)

Scaffolding landed.

- Moved 45 pre-refactor files (all `.c/.h/.prg/.o/.bin/.s/.zip/.d64/.com/.dsk/.png/.sh`
  + `demo/ python/ rasterscroll/ src/ welcome/ __pycache__/`) into
  `ARCHIVE_pre_refactor/` with README banner "Do not edit."
- `platform/platform.h` — frozen 15-function API: `plat_init/shutdown`,
  `plat_screen_w/h`, `plat_cls/putc/puts`, `plat_key_pressed/wait`,
  `plat_delay_ms`, `plat_rand/seed_rand`, `plat_beep`. K_* key codes
  (UP/DOWN/LEFT/RIGHT/FIRE/QUIT/OTHER/NONE). COL_* colour enums
  (BLACK/WHITE/RED/GREEN/BLUE/YELLOW/CYAN/MAGENTA).
- `game/glyphs.h` — G_FLOOR/WALL/DOOR/PLAYER/ENEMY/GOLD/POTION/WEAPON/
  STAIRS/CORPSE/SPACE/BORDER (G_COUNT=12). Why: symbolic glyphs keep
  game code ignorant of PETSCII/ATASCII/tile# differences across targets.
- `platform/plat_c64.c` — cc65 + conio adapter. PETSCII → K_* key
  translation (cursor chars 145/17/157/29, plus WASD fallback), xorshift16
  RNG, empty `plat_beep` stub.
- `game/main.c` — smoke test: draws `@` centered, arrows/WASD move, Q quits.
  No map, no enemies yet — just proves the adapter plumbing.
- `build/common.mk` + `build/Makefile.c64` — `cl65 -t c64 -O -Cl`.
- `README.md` rewritten with new layout + golden rules + roadmap.

### 2026-04-23 — Phase 2 (commit `1c0f1bb`)

Map module migrated.

- `game/map.h` — MAP_MAX_W/H constants, game_object_t, extern tile grid,
  `map_load/get/is_solid`. Why: `MAP_MAX_*` not `40/25`; sizing decided
  at init from `plat_screen_w/h`.
- `game/map.c` — room 0 ported from `ARCHIVE_pre_refactor/map.h` (24 rows
  × 40 cols, hand-drawn). `ascii_to_glyph()` converts `#.+@KGRH$/%I P>`
  raw chars into G_* at load time — game never sees ASCII after this.
  Template markers (`@`, enemies, items) collected into `map_game_objects[]`
  array with glyph id. Why: player start + entity positions encoded in
  the template; parsing once at load simplifies downstream code.
- `game/main.c` — draws map via `plat_putc` with per-glyph colour, then
  draws game objects on top, then player. Movement checks `map_is_solid()`.
- `build/common.mk` — added `game/map.c`.
- `platform/platform.h` — fixed nested `/*` in header comment.
- C64 PRG: 3467 bytes.

### 2026-04-23 — Phase 3 (commit `0dad94d`)

Entities, combat, pickups, status bar.

- `game/entity.h/c` — `entity_t` (x, y, glyph, alive, hp, dmg). Built
  from `map_game_objects[]` at init: enemies hp=3/dmg=1, items hp/dmg=0.
  - `entity_at(x, y)` → index or -1
  - `entity_kill(idx)` marks dead + writes G_CORPSE to map
  - `entity_ai_turn(px, py)` — greedy axis-first step, fallback secondary
    axis if blocked. No line-of-sight yet.
  - `entity_adjacent_damage(px, py)` — 8-way Chebyshev ≤1, sums dmg.
- `game/main.c` turn-based loop: move → bump/pickup → enemy AI →
  adjacent damage → status bar. `u8_to_str()` formats "HP:n GLD:n DMG:n".
  "YOU DIED" screen at HP 0.
- `game/map.c` — added `map_set`. `map_is_solid` now rejects G_DOOR
  (doors block until unlocked — future work).
- `build/common.mk` — +`game/entity.c`.
- C64 PRG: 6213 bytes.

### 2026-04-23 — Phase 3.1 (commit `dfbff91`)

Incremental redraw. Stops full-screen repaint each turn.

- `game/entity.h` — `move_event_t {ox, oy, nx, ny, g}` buffer +
  `move_event_count`. Why: `entity_ai_turn` records moves without
  drawing, main loop dispatches the redraws.
- `game/entity.c` — `record_move()` fills buffer before mutating
  `entities[i].x/y`. Buffer cleared at start of `entity_ai_turn`.
- `game/main.c`:
  - `initial_render()` draws map + game objects + player once. Never again.
  - `redraw_cell(x, y)` generic: show live entity if present, else map.
  - Per turn: player old + new cell, killed enemy → G_CORPSE,
    each `move_events[i]`: old + new cell.
  - `redraw_status_if_changed()` — only repaint status row when stats
    differ from `prev_hp/gold/dmg`. Why: `cputsxy` is slow on CBM.
  - Fixed bug: `K_DOWN` now reaches `map_h - 1` (was `-2`, unreachable row).
- `game/map.c` — `map_load` reserves last row for status bar.
- C64 PRG: 7013 bytes (+800). Worth it — no flicker.

### 2026-04-23 — Phase 4 (commit `d54855f`)

Widened to 7 platforms.

- `platform/plat_pet.c` — mono PET, cc65 conio, 40×25.
- `platform/plat_plus4.c` — TED, cc65 conio, COLOR_VIOLET (plus4/vic20
  lack COLOR_PURPLE alias that c64.h has).
- `platform/plat_vic20.c` — 22×23. Map auto-clips via
  `map_w = min(plat_screen_w(), ROOM_W)`. Why: 40×24 room template
  shows only left 22 cols on VIC-20; partial view still playable.
- `platform/plat_apple2.c` — 40×24 mono. Ctrl-HJKL arrows (8/21/11/10).
- `platform/plat_atari8.c` — 40×24, ATASCII cursor 28/29/30/31, EOL 155.
- `platform/plat_host.c` — ncurses dev adapter. `plat_seed_rand(0)`
  uses `time(NULL)`.
- Makefiles: pet/plus4/vic20/apple2/atari8/host. vic20 Makefile uses
  `-C vic20-32k.cfg` (local only; didn't change shared default).
- Build sizes: c64 7013 / pet 6988 / plus4 7159 / vic20 6994 /
  apple2 7011 / atari.xex 7266 / host 36K mach-o.

### 2026-04-23 — Phase 4.1 (commit `87e6ac5`)

BBC adapter + cc65 port quirk.

- `platform/plat_bbc.c` — BBC Micro, 40×25, mono. `plat_puts` rolls
  its own via `cputcxy` loop. Why: cc65 BBC port in this toolchain
  doesn't ship `cputsxy/gotoxy`. Noted in MAINTENANCE.md.
- `build/Makefile.bbc` — outputs `.ssd`.
- Same `cputsxy`-free fix applied to `8bitworkshop/presets/bbc/retroc-dungeon.c`.

### 2026-04-23 — IDE integration (8bitworkshop repo)

Not a retro-c commit; parallel work in `/Users/chrisg/github/8bitworkshop`.

- `presets/<plat>/retroc-dungeon.c` for c64, pet, plus4, apple2, apple2e,
  atari8-800, bbc. Each is a hand-concatenated game + adapter inline.
- Filed under preset category **"retro-c cross-platform demo"**.
- VIC-20 preset **not** shipped — demo overflows default unexpanded
  `vic20.cfg` by ~2.5 KB. Changing default to `vic20-32k.cfg` globally
  would affect other presets, so deferred. Revisit with `TIER_MINIMAL`.
- Not shipped: nes, gb, atari5200, atari7800, c128 (Phase 5 or later).

### 2026-04-23 — Maintenance docs

- Added `docs/MAINTENANCE.md` — golden rules, build matrix, test
  checklist, debugging guide, procedure for adding features and new
  adapters, procedure for regenerating IDE single-file demos.

### 2026-04-23 — Phase 4.2 (commit `5abf231`)

Enemy type table + HP/magic/idol pickup semantics corrected per author.

- `game/enemy_types.h` + `game/entity.c`: introduced `ENEMY_TYPES[]`
  static table (marker, hp, dmg, speed, armour, colour, name). Types:
  goblin (G, hp 30, green), rat (R, hp 15, cyan), kobold (K, hp 20,
  magenta). Horror dropped — H marker repurposed.
- `game/glyphs.h`: +G_HEALTH (12), +G_MAGIC (13), +G_IDOL (14).
  G_COUNT bumped 12 → 15. All 7 adapters' `glyph_native[]` extended
  with `'h','*','i'` (magic pickup `*`; same char reused for G_BOLT/G_BORDER on some targets).
- `game/map.c`: `ascii_to_glyph` revised per author's intent —
  H = health pickup (was enemy); templates later settled on `*` = magic
  (G_MAGIC), `P` = potion (G_POTION). I = idol. game_object_t carries type_idx.
- `game/entity.h/c`: +player_magic, +player_idols, +idols_total.
  Entity init reads stats from ENEMY_TYPES[type_idx]. Player stats
  scaled to match: hp 30, dmg 10.
- `game/main.c`: enemies drawn in own type colour (not generic red).
  step_onto pickups: health +10 hp, magic +1 mp, idol increment,
  weapon +2 dmg. Status bar rewritten to 40 cols: "HP:n MP:n $:n
  I:n/N DMG:n". Win state "ALL IDOLS FOUND" when player_idols ≥
  idols_total.
- `colour_for_glyph`: G_HEALTH = RED, G_MAGIC = MAGENTA, G_IDOL = YELLOW.
- `scripts/bundle-ide-demo.py`: written. Concatenates `game/*.c` +
  `platform/plat_<t>.c` into a single-file IDE preset. Strips local
  `#include`, dedupes system headers. Verified c64 bundled build
  produces byte-identical 8117-byte PRG vs modular build.
- Ran bundle script for all 7 IDE presets (c64/pet/plus4/apple2/
  apple2e/atari8-800/bbc).
- C64 PRG: 8117 bytes.

### 2026-04-23 — Phase 4.3 (commit `ccd777b`)

Original combat + movement mechanics ported verbatim from
`ARCHIVE_pre_refactor/dungeon_multi.c` (attack(), enemy_attack(),
move_enemies() functions).

- `game/entity.c`: `entity_player_attack()` rolls d20 (`plat_rand%20+1`).
  Hit if `rnum > armour + speed`. On hit: enemy.hp -= weapon. On miss +
  player 4-way-adjacent: enemy lands free `strength` hit (original's
  "revenge on miss" mechanic). Kills leave G_CORPSE on map.
- `game/entity.c`: `enemy_attack_player()` rolls d20; >10 → enemy hits
  for `strength`, ≤10 → player counter-attacks for flat 5 damage.
- `game/entity.c`: `entity_ai_turn()` rewritten per type:
  - **Rat** (type_idx 1): random NSEW step (`rand%4+1` → 1=up, 2=right,
    3=down, 4=left).
  - **Goblin + Kobold**: both axes toward player simultaneously
    (diagonal-capable chase).
  - Blocked target: stay put. Target cell = player: invoke
    `enemy_attack_player` instead of recording move.
- `entity_adjacent_damage` retained as no-op stub for API back-compat;
  damage now applied inside AI turn via enemy_attack_player. Main loop
  post-AI just checks `player_hp == 0` for death.
- `game/main.c`: `step_onto` routes enemy bump into
  `entity_player_attack` (was flat-damage kill). Player stays in place
  on bump — attack consumes the turn whether hit or miss.
- Hit probabilities emerge from type stats: goblin needs d20>11 (45%),
  rat needs d20>2 (90%), kobold needs d20>6 (70%).
- C64 PRG: 8395 bytes (+278).

### 2026-04-23 — Phase 4.4 (commit `372e03b`)

Fireball spell ported from original (case 'f' in dungeon_multi.c).

- `game/glyphs.h`: +G_BOLT (index 15). G_COUNT 15 → 16. All 8 adapters'
  `glyph_native[]` extended with `'*'`.
- `game/main.c`:
  - `direction_x`, `direction_y` statics — updated on every player
    move (default east). Matches original globals.
  - `cast_fireball(px, py)` — requires `player_magic > 5`, costs 5 MP
    up front. Walks along (direction_x, direction_y), animating
    G_BOLT at each floor cell with `plat_delay_ms(80)`, consuming 1
    MP per tile. Stops on non-floor OR entity OR MP depleted.
    On impact: `entity_player_attack(weapon=10)` same d20 roll as
    bump attack. Kills leave G_CORPSE.
- `K_FIRE` (Z / space / return) now casts instead of being no-op.
- `colour_for_glyph`: G_BOLT = COL_YELLOW.
- C64 PRG: 8917 bytes.

### 2026-04-23 — Phase 4.5 (commit `97b1545`)

AI wake range + keys/doors + BBC scroll fix. Ported from raylib version.

- `game/entity.c`: `within_wake_range()` — squared-distance check vs
  `AI_WAKE_RANGE` (6 tiles). Enemies outside range skip their AI turn.
  Mirrors raylib's `is_within_range(player, enemy, 6)`. Big gameplay
  improvement: distant enemies stay asleep, approach triggers combat.
- `game/glyphs.h`: +G_KEY (16). G_COUNT 16 → 17. All 8 adapters'
  `glyph_native[]` extended with `'k'`.
- `game/map.c`: `ascii_to_glyph` maps `k` → G_KEY pickup. Added one
  `k` to room 0 so the 4 existing doors become reachable.
- `game/entity.h/c`: +`player_keys` counter, reset to 0 on map load.
- `game/main.c`: `step_onto` door branch — target G_DOOR consumes one
  key and converts map cell to G_FLOOR in-place (`map_set` +
  `plat_putc`). No key = block as before. Corresponds to raylib
  `'+'` → `'-'` (partially open, passable) state.
- `game/main.c`: pickup switch handles G_KEY.
- Status bar rewritten: `HP:n MP:n $:n K:n I:n/N`. DMG field dropped
  (static once weapons collected; keys more actionable info).
- **BBC scroll bug fix**: `map_load` now reserves 2 bottom rows (was 1).
  Status draws at row `map_h`; row `map_h+1` stays empty so BBC cc65
  conio does not scroll when chars land on the final row.
- C64 PRG: 9379 bytes (was 8917, +462 for wake range math + keys
  logic + status field).

### To-do (next passes)

#### Game features from raylib version (omiq/Raylib-Dungeon)

Comparison of raylib version shows same 2 enemy types (goblin, rat —
we've added kobold on top) but several mechanics the 8-bit port
doesn't have yet:

- **AI wake range** — `is_within_range(player, enemy, 6)`: enemies
  only move when player is within 6 tiles. Currently enemies chase
  from anywhere on the map. One-line change in `entity_ai_turn`.
- **Keys + doors** — `'k'` pickup grants a key, `'+'` (closed door)
  consumes a key to open into `'-'` (partially open, passable).
  Already have G_DOOR + G_KEY would be new glyph.
- **Sword pickup** — distinct from generic weapon upgrade; bumps
  bump damage from 10 to higher value when equipped.
- **`placeObject()` procgen** — random placement of enemies/items
  based on current dungeon level; counts scale with depth.
- **Fog of war / visibility map** — `visibility_map[]` tracks
  explored cells; unseen drawn as space.
- **Partial door state** (`'-'`) — passable after keyed.

#### Portability / infrastructure

- **Phase 5**: GB (GBDK tile-based adapter) + NES (cc65 nes nametable).
  Real portability test — API leaks 40×25 assumptions crack here.
- **Phase 6**: delete `ARCHIVE_pre_refactor/` after every salvageable
  asset has been migrated (charset-c64 data, maze.h gen, cpm if kept).
- VIC-20 revisit: add `TIER_MINIMAL` flag shrinking tables so
  unexpanded `vic20.cfg` (3.5 KB MAIN) build fits. Without it, the
  demo overflows default config by ~2.5 KB.
- Procedural map gen (maze.h in archive); multi-room via G_STAIRS
  (already parsed, needs `map_load(next_room)`); title/menu screen;
  sound via `plat_beep` (currently no-op in every adapter).

#### Known unshipped IDE targets

- nes, gb, atari5200, atari7800, c128 — not yet ported. All would be
  Phase 5 or later.

## Reference — raylib/modern rewrites

Author maintains modern rewrites using raylib (not cc65). Different
portability story — these use raylib's graphics+audio+input, so game
logic is portable across raylib-supporting platforms (desktop,
WebAssembly via emscripten) but renderer/asset code does not map
back onto our 15-fn text-mode `platform.h`.

- `github.com/omiq/Raylib-Dungeon` — native raylib build
- `github.com/omiq/Raylib-Emscripten-Dungeon` — same game, WASM web build
- `makerhacks.itch.io/retro-rogue-dungeon` — published game page

Useful as a source of game design / balance / AI tuning references
when porting features into `game/*.c`. Rendering cannot transplant;
game logic can (as done with attack / enemy_attack / move_enemies
ports in Phase 4.3 + fireball in 4.4).
