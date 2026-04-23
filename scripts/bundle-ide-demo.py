#!/usr/bin/env python3
"""
Concatenate game/*.c + platform/plat_<plat>.c into a single .c file for the
8bitworkshop IDE preset slot. Usage:
    scripts/bundle-ide-demo.py c64 > /tmp/c64.c
    scripts/bundle-ide-demo.py plus4 /path/to/8bitworkshop/presets/plus4/retroc-dungeon.c

Rules:
- strip all #include "..." of local headers/sources (pasted inline instead)
- keep system headers (<stdint.h>, <conio.h>, <ncurses.h>, ...)
- inject header contents (glyphs.h, enemy_types.h, platform.h, map.h, entity.h)
  before the source body. Definitions of extern vars stay in entity.c.
- order: system headers -> inlined project headers -> adapter -> map -> entity -> main
"""
import re
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
LOCAL_INCLUDE = re.compile(r'#\s*include\s+"([^"]+)"')

def read(rel):
    return (ROOT / rel).read_text()

def strip_local_includes(src):
    return LOCAL_INCLUDE.sub("", src)

def pluck_system_includes(src):
    """Collect #include <...> lines, return (system_headers_block, rest)."""
    sys_re = re.compile(r'(#\s*include\s+<[^>]+>\s*\n)')
    sys_lines = sys_re.findall(src)
    rest = sys_re.sub("", src)
    return "".join(sys_lines), rest

def bundle(plat):
    headers = [
        read("game/glyphs.h"),
        read("game/enemy_types.h"),
        read("platform/platform.h"),
        read("game/map.h"),
        read("game/entity.h"),
    ]
    bodies = [
        read(f"platform/plat_{plat}.c"),
        read("game/map.c"),
        read("game/entity.c"),
        read("game/main.c"),
    ]

    all_sys = []
    cleaned_parts = []
    for part in headers + bodies:
        part = strip_local_includes(part)
        sys_block, rest = pluck_system_includes(part)
        all_sys.append(sys_block)
        cleaned_parts.append(rest)

    # dedupe system headers preserving order
    seen = set()
    sys_out = []
    for block in all_sys:
        for line in block.splitlines(keepends=True):
            if line.strip() and line not in seen:
                seen.add(line)
                sys_out.append(line)

    return (
        f"/* Auto-generated from retro-c branch master.\n"
        f" * Source: game/*.c + platform/plat_{plat}.c.\n"
        f" * DO NOT EDIT in place — regenerate via scripts/bundle-ide-demo.py {plat}. */\n\n"
        + "".join(sys_out)
        + "\n"
        + "\n".join(cleaned_parts)
    )

def main():
    if len(sys.argv) < 2:
        print(__doc__, file=sys.stderr)
        sys.exit(1)
    plat = sys.argv[1]
    out = bundle(plat)
    if len(sys.argv) >= 3:
        pathlib.Path(sys.argv[2]).write_text(out)
        print(f"wrote {sys.argv[2]} ({len(out)} bytes)")
    else:
        sys.stdout.write(out)

if __name__ == "__main__":
    main()
