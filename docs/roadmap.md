# Next steps

## Confirmed on PSP

- NES and GBA launch directly from their generated Memory Stick entries.
- Version 1.01 displays game-specific cover icons.
- Version 1.02 displays borderless square cover icons.

## Current candidate: 1.03

Add a full-screen PIC1 background generated from each existing cover.
Confirm that selecting an entry displays the background and that the game
still starts normally, for both NES and GBA. Until confirmed, 1.02 remains
the stable fallback.

## Before adding more systems

1. Verify GBA save isolation between two games using the shared temporary
   `game.gba` runtime. Add per-ROM save routing if necessary, retaining backups.
2. Make installation and regeneration predictable: clear missing-file errors,
   automatic removal of stale entries owned by GameShelf, and a documented
   recovery procedure.
3. Check third-party license notices and corresponding emulator sources for
   the distributed binaries before wider promotion. No license change is assumed.

## Expansion

Add one direct-ROM emulator adapter at a time, beginning with GB/GBC or Atari
according to user preference. Existing catalog entries that only open emulator
menus must not be presented as working direct launch adapters.

## Public demonstration

After confirming 1.03 and save behavior, record a short PSP demonstration:
Memory Stick, icon and background, then direct NES and GBA launch. Provide a
small installation guide, exact tested firmware, credits and known limitations.
Use community feedback to choose the next emulator integration.
