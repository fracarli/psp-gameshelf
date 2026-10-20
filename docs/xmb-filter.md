# Optional XMB emulator filter

GameShelf launches shared engines by their original paths. Hiding their menu
entries can make Game > Memory Stick easier to browse without moving or
removing executables, ROMs, settings or saves.

The optional configuration uses the original
[Game Categories Lite 1.6 release](https://github.com/codestation/gclite/releases/tag/1.6)
by Codestation. Its [source](https://github.com/codestation/gclite/tree/1.6)
retains GPLv3-or-later terms. This is an upstream plugin, not a GameShelf emulator
or a feature of the stable GameShelf executable.

It was installed on the existing PSP-3004 / 6.61 PRO-C card on 2026-10-20.
File writes were verified; its appearance and launch behavior still need a
hardware check. It is not included in the stable first-install ZIP.

## Configuration used

The card had no existing seplugins configuration or CAT_ categories. Other
setups require a merge that preserves existing plugins and category settings.

The official category_lite.prx is placed in seplugins. Its line is first in
seplugins/vsh.txt, as required by upstream:

```text
ms0:/seplugins/category_lite.prx 1
```

The gclite_filter.txt file contains exact directory names, one per line,
including the final newline:

```text
Nester_AoEX_R3
TempGBA4PSP-Mod
GBAdhoc410
psp2600
PicoDrive
s9xTYLme_mod
```

The 20-byte gclite.bin file stores five little-endian unsigned integers:
mode=0 (Multi MS), prefix=1 (CAT_), uncategorized=1 (Memory Stick), selection=0,
catsort=0. With no CAT_ folders this retains one directly accessible game list.
The upstream English translation file has its single Uncategorized label
changed to Memory Stick; all other text is retained. Other system languages
may show the default Uncategorized label.

GameShelf, FastRecovery, Minecraft and DaedalusX64 remain visible. The first two
provide maintenance/recovery, and N64 games still require DaedalusX64. Generated
GSH_ entries, PSP ISOs and PS1 titles are not in the filter.

## Activation, hardware check and rollback

Safely eject the card, leave USB mode, press SELECT in XMB and choose RESET VSH.
Check that the six emulator entries are hidden, generated game entries remain,
and NES/GBA/GB/GBC/Atari/Mega Drive launches and existing saves still work.
Check any available SNES titles and the normal PSP/PS1 entries too.

To disable the filter, change the line above to end in 0, then reset VSH again.
If XMB is not usable, edit vsh.txt with a card reader. If there were no other
plugins, removing just that line also disables it. Do not delete runtime folders.
The existing card's installation record and original release archive are kept
in the private computer backup directory, outside this repository.

[FREESTOR's own documentation](https://github.com/GorGylka/FREESTOR) lists a
conflict with category_lite.prx. Review this before combining them.

## Verified upstream file hashes

- categories_lite-1.6.zip: c3b6daa06523e6d17140e0367c91d96c835afc3bbd2792f57deb3b6b47ddd852
- category_lite.prx: 576d2ff2e9fdfc4cdef7988ccbb1fe421b4caf57c23a01971363bd5b0d1ed717
