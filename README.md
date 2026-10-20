# PSP GameShelf

Launch NES and GBA games directly from **Game → Memory Stick** on your PSP.
Select a game and its emulator loads it automatically, without opening a
separate game library or selecting the ROM again inside the emulator.

**Direct NES and GBA launch has been confirmed on a real PSP.**
Tested hardware: PSP-3004, firmware 6.61 with PRO-C enabled. After a full
shutdown, FastRecovery may be needed before starting homebrew.

## Versions

| Version | Status | Features |
| --- | --- | --- |
| [0](releases/v0/00/) | Stable baseline | Launch from the catalog; previously v0.51.0 |
| [1.00](releases/v1/00/) | NES/GBA launch confirmed on PSP | Individual game entries in the Memory Stick menu |
| [1.01](releases/v1/01/) | Cover icons confirmed on PSP | Game-specific cover icons |
| [1.02](releases/v1/02/) | Confirmed on PSP | Square icons without a rectangular frame |
| [1.03](releases/v1/03/) | Backgrounds confirmed on PSP | Full-screen background when a game is highlighted |
| [1.04](releases/v1/04/) | Separate saves confirmed on PSP | Per-game GBA save, state and configuration filenames |
| [1.05](releases/v1/05/) | English UI confirmed on PSP | PSP system-language selection and English fallback |
| [1.06](candidates/1.06/) | Awaiting hardware confirmation | Loading feedback, one-copy GBA staging, recoverable stale-entry cleanup |

`releases/` contains hardware-confirmed builds and the working NesterJ adapter.
Experimental builds are kept separately in `candidates/`. Obsolete packages
have been removed from the current tree; tracked versions remain in Git history.

## Getting started

1. Follow the [1.00 installation instructions](releases/v1/00/INSTALL.txt).
2. Install the [stable 1.03 launcher](releases/v1/03/INSTALL.txt) for square
   cover icons and full-screen backgrounds. Emulators remain shared.
3. Open GameShelf and press **X** to generate or update game entries.
4. When finished, press **X** to return to the PSP home screen, then open
   **Game → Memory Stick**.
5. Select a game to start it directly. Generated entries persist after reboot.

Press **Circle** on the initial GameShelf screen to open the legacy catalog.

| System | ROM directory | Required emulator |
| --- | --- | --- |
| NES | `PSP/GAME/Nester_AoEX_R3/roms/*.nes` | NesterJ with the autoload adapter |
| GBA | `PSP/GAME/TempGBA4PSP-Mod/roms/*.gba` | Integrated TempGBA under `PSP/GAME/GAMESHELF/TempGBA/` |

Keep the adapted emulator builds: standard upstream executables do not provide
all the startup behavior required by GameShelf.

## Covers and backgrounds

With your memory card mounted on your computer:

```sh
python3 tools/fetch_boxart.py --sd-root /Volumes/SD-FAT32 --platform nes --platform gba
```

The helper downloads available covers and prepares these assets under
`PSP/GAME/GAMESHELF/boxart/{nes,gba}/`:

- `Game.gsh`: legacy catalog texture.
- `Game.xmb.gsh`: square cover texture for an 80×80 XMB icon.
- `Game.pic1.png`: 480×272 full-screen background.

Asset names must match the ROM filename without its extension. Square images
fill the icon; rectangular images are cropped centrally without stretching.
The background is an enlarged cover crop, darkened to keep XMB text readable.
Highlight a game and wait for the XMB to display its background.

Games without matching art retain the default GameShelf icon. ROM hacks may
need manually supplied artwork. Downloaded images stay on the memory card
and are not included in this repository.

To update the stable launcher with an automatic backup:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.03 --backup-root ../PSP_BACKUP
```

This is an update helper; emulator runtimes must already be installed.

## GBA save isolation in 1.04 — confirmed on PSP

Version 1.04 must update **both GameShelf and the integrated TempGBA runtime**:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.04 --backup-root ../PSP_BACKUP
```

The installer creates a complete backup, then backs up and removes old GBA
entries because each entry contains the previous launcher. Open GameShelf on
the PSP and press **X** to regenerate them before playing.

The staged ROM remains `game.gba`, but TempGBA receives the original ROM name
through `SAVE_ID.TXT`, checked against the loaded ROM's size and header.
Saves, states, game settings and screenshots use the original name in the
existing `GAMESHELF/TempGBA/` directories. ROM names are limited to 180 bytes;
renaming a ROM also changes its associated save filename.

Existing shared saves such as `game.sav` and old states are preserved without
automatically assigning them to a game. Identifying the correct title is
required before transferring an old shared save.

Test on hardware: **game A → save → game B → save → reopen A**, then reopen B.
The user confirmed independent Mario Kart and Zelda saves. Version 1.04 is
the stable save-isolation build; further save-state testing remains useful.

To roll back, restore the complete backup of GameShelf, TempGBA and generated
GBA entries. The installer rejects a launcher-only downgrade from 1.04.

## Startup and installation improvements in 1.06 — experimental

```sh
# Read-only checks first; no backup directory is needed for this command.
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --check
# Then update, with a complete backup.
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.06 --backup-root ../PSP_BACKUP
```

Checks run before writes and report missing directories, invalid PBP headers,
missing support files and an unexpected BIOS size. This is an updater for an
existing NES/GBA setup; it does not supply games or BIOS files.

Game startup shows preparation stages and ROM-copy progress. GBA preparation
copies the ROM once instead of twice; unchanged support modules are not
rewritten. TempGBA also displays loading messages. Some delay during firmware
handoff remains possible. Hardware timing comparisons have not been measured.
`GAMESHELF/STARTUP_LOG.TXT` records preparation time and copied-file bytes only.

When you generate entries with X, 1.06 checks for missing ROMs and moves verified
GameShelf entries to `PSP/GAMESHELF/REMOVED_ENTRIES`. It never deletes saves or
ROMs. Existing ROMs beyond the scanner limit remain protected. Unknown entries,
entries containing extra user files and unavailable libraries are skipped.
Restore a moved ROM and regenerate to recreate its entry.

## Language

To install the stable 1.05 language update, preserving saves and backing up old entries:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.05 --backup-root ../PSP_BACKUP
```

Open GameShelf 1.05 and press X to regenerate entries after the update.


Project documentation and computer tools use English. The PSP launcher
reads the console's system language: English and Italian are supported, with
English as the fallback for all other languages or a failed settings read.
Emulator menus retain the language options provided by their upstream projects.

## Current limitations

- Up to 32 ROMs per system, scanned from the expected directory without recursion.
- Each game entry contains a small copy of the launcher; emulator runtimes are shared.
- Through 1.05, entries for deleted ROMs must be removed manually. The 1.06
  candidate archives verified missing-ROM entries automatically during regeneration.
- Exit GBA games through the emulator's **Quit/Exit** menu. The firmware HOME
  popup remains disabled in the integrated runtime.
- Versions through 1.03 may share the GBA save filename `game.sav`.
  Separate save filenames in 1.04 have been confirmed on PSP.
- PSP ISO and PS1 games work through the normal XMB, but launching them through
  GameShelf remains unresolved on the tested PRO-C installation.
- GB/GBC, Atari, Mega Drive and SNES appear in the legacy catalog; automatic
  ROM selection inside those emulators is not implemented yet.

## Source and verification

```sh
git clone --recurse-submodules https://github.com/fracarli/psp-gameshelf.git
cd psp-gameshelf
# Install PSPDEV/PSPSDK and put its tools on PATH first.
make
python3 tools/verify_xmb.py
python3 tools/verify_gba_saves.py
python3 tools/verify_install.py
python3 tools/verify_language.py
python3 tools/verify_startup.py
python3 tools/verify_cleanup.py
```

The Makefile defaults to `~/pspdev`; set `PSPDEV` for a different installation.
The current source builds the 1.06 candidate. Its TempGBA changes add loading
messages; the hardware-confirmed 1.04 save identity and emulation logic remain. Tests cover generated PBP/SFO
metadata, executable preservation, cover and background PNGs, invalid inputs,
per-game save identity, paired installation and rollback, and language selection.
Computer tests do not replace testing on a real PSP.

Emulator sources are tracked as upstream submodules. The adaptations used by
this project are in [patches/](patches/README.md).
See the [version 0 technical history](docs/history-v0.md) and
[roadmap](docs/roadmap.md) for background and next steps.

## Published contents and licenses

This repository contains only PSP GameShelf code, documentation and release
packages. It excludes ROMs, ISO images, BIOS files, saves, downloaded artwork,
memory-card backups and unrelated files from the developer's computer.

GameShelf code: [MIT](LICENSE). Third-party emulators retain their upstream
licenses; the 1.04 TempGBA package includes its GPL text and source references.
