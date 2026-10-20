# PSP GameShelf

Your retro games, directly in **Game → Memory Stick**.
Choose a game and it opens in its shared emulator without selecting the ROM again.
Square cover icons and full-screen backgrounds are supported.

**Confirmed on hardware:** NES/GBA launch, covers, backgrounds, separate GBA
saves, English UI and loading feedback. Tested on PSP-3004, 6.61 PRO-C.
**GB/GBC direct launch is also confirmed on hardware in 1.07.**
Atari 2600, Mega Drive and SNES are implemented in the **1.08 test build**.

## First installation

You need a PSP that **already runs homebrew with custom firmware**. GameShelf
does not install or change firmware. With PRO-C, FastRecovery may be needed
after a full shutdown. Back up your memory card before copying files.

1. Download [GameShelf 1.07 setup ZIP](https://github.com/fracarli/psp-gameshelf/raw/refs/heads/main/releases/v1/07/GameShelf-1.07-setup.zip)
   and extract it. This is the latest hardware-confirmed launcher/runtime.
2. Copy the extracted **PSP** folder to the **root of the memory card**,
   merging with its existing PSP folder. Do not replace the whole PSP folder.
3. Put your own games and, for GBA, your own BIOS in the paths below.
4. Safely eject the card. On PSP, open **Game → Memory Stick → PSP GameShelf**.
   Press **X** to create entries, review the summary, then **X** to return home.
5. Open **Game → Memory Stick** and select a game.

**Correct:** `[card]/PSP/GAME/GAMESHELF/EBOOT.PBP`

**Wrong:** `[card]/PSP/PSP/GAME/GAMESHELF/EBOOT.PBP`

No Python, compiler or previous GameShelf versions are required for this first
installation. The ZIP includes the adapted emulator executables and support
modules. It contains **no games, BIOS, saves or downloaded artwork**.
The assembled first-install ZIP has computer checks; a fresh-card hardware
installation still needs confirmation. The individual NES/GBA runtimes have
already been tested on the existing PSP setup.

| Files you provide | Location on the card |
| --- | --- |
| NES `.nes` games | `PSP/GAME/Nester_AoEX_R3/roms/` |
| GBA `.gba` games | `PSP/GAME/TempGBA4PSP-Mod/roms/` |
| GBA BIOS, named `gba_bios.bin`, 16,384 bytes | `PSP/GAME/TempGBA4PSP-Mod/` |
| GB `.gb` / GBC `.gbc` games, **1.07 only** | `PSP/GAME/TempGBA4PSP-Mod/roms/` |

NES and GB/GBC do not require the GBA BIOS. Scan limit: **32 games per system**,
without subfolders. ROM names determine save associations: keep names unchanged.
Run GameShelf again after adding/removing games, adding art or updating the app.
Generated entries persist after reboot.

See [START-HERE.txt](docs/START-HERE.txt) for troubleshooting and save locations.

## Updating an existing installation

Use the backup-preserving computer updater instead of merging a first-install
ZIP over an existing setup. It requires Python 3.9 or newer:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.07 --check
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.07 --backup-root ../PSP_BACKUP
```

Replace `/Volumes/SD-FAT32` with your card path, for example `E:/` on Windows.
Checks happen before writes. The updater backs up GameShelf and old GBA entries,
updates launcher and runtime together and verifies writes. Saves are preserved.
Open GameShelf and press **X** to regenerate entries after updating.
Restore the complete backup to roll back; do not downgrade only one executable.

## Additional emulators: 1.08 preview

[Download the 1.08 test setup ZIP](https://github.com/fracarli/psp-gameshelf/raw/refs/heads/main/candidates/1.08/GameShelf-1.08-setup.zip), or
use the updater above with `--version 1.08` for an existing installation.
Read the [1.08 instructions](candidates/1.08/INSTALL.txt).

| System | ROM directory | Uncompressed extensions |
| --- | --- | --- |
| Atari 2600 | `PSP/GAME/psp2600/roms/` | `.a26`, `.bin`, `.rom` |
| Mega Drive / Genesis | `PSP/GAME/PicoDrive/roms/` | `.md`, `.bin`, `.gen`, `.smd` |
| SNES | `PSP/GAME/s9xTYLme_mod/roms/` | `.smc`, `.sfc` |

The Atari/SNES adapters receive the original ROM path and skip ROM selection.
PicoDrive 2.05 already supports direct ROM arguments; its official PSP binary is
verified and reused without modification. ROMs are read in place, with no extra
staging copy. Original save/state/configuration locations remain in use.
Paths for these systems must fit within 239 bytes including the directory.

The updater backs up the **complete Atari and SNES folders**, then replaces
only their executables alongside the GameShelf update. Existing support modules,
ROMs and settings are retained. The first-install ZIP supplies the engines,
modules and required keyboard assets; the optional upstream music archive is omitted.

A new original bridge icon is used in this preview. The public name and card
paths remain GameShelf while a new identity is being considered.

Computer checks pass; **Atari/MD/SNES startup, controls and saves still need PSP
confirmation**. Version 1.07 remains the confirmed fallback. GB/GBC startup was
confirmed with multiple games; dedicated save/state checks remain useful.

## Covers and backgrounds

Optional: games launch without artwork. With Python 3.10+ installed and the card
connected, run:

```sh
python3 tools/fetch_boxart.py --sd-root /Volumes/SD-FAT32 --platform nes --platform gba
# Also supported: --platform gb --platform gbc --platform atari --platform megadrive --platform snes
```

The helper fetches available Libretro covers onto the card and prepares square
icons and darkened 480×272 backgrounds in `GAMESHELF/boxart/{nes,gba,gb,gbc,atari,megadrive,snes}/`.
Names must match the ROM basename. Rectangular covers are centrally cropped.
Run GameShelf and press X after changing artwork. Highlight a game and wait
for XMB to display its background. ROM hacks may need manually supplied art.
Downloaded art stays on the card and is never part of this repository.

## Saves, loading and library cleanup

GBA saves have used separate original-game filenames since 1.04, confirmed
with Mario Kart and Zelda. Use the GBA emulator's **Quit/Exit** menu; the
firmware HOME popup is disabled in that runtime. Existing shared `game.sav`
files are preserved without assigning them to a title automatically.

Version 1.06 copies each GBA ROM once and avoids rewriting identical support
files. The user reports working loading feedback and possibly shorter startup;
no speed difference has been measured. `GAMESHELF/STARTUP_LOG.TXT` records
launcher preparation only, excluding firmware/emulator hand-off.

During regeneration, verified GameShelf entries whose ROMs are missing are
moved intact to `PSP/GAMESHELF/REMOVED_ENTRIES`. ROMs and saves are preserved.
Unknown entries, extra user files and unavailable libraries are skipped. This
cleanup has computer tests and still needs a dedicated PSP test. Restore the
ROM and regenerate to recreate its entry.

## Supported systems and limits

| System | Individual XMB entry and direct ROM selection |
| --- | --- |
| NES / GBA | Confirmed on PSP |
| GB / GBC | Direct launch confirmed on PSP in 1.07 |
| Atari / Mega Drive / SNES | Implemented in 1.08; awaiting PSP tests |
| PSP ISO / PS1 | Use the normal PSP home screen; GameShelf launch remains unresolved on tested PRO-C |

Each entry contains a small launcher copy; emulator runtimes remain shared.
Press Circle on the initial screen to open the legacy catalog. Project docs
and tools are English. The PSP launcher follows English/Italian system-language
settings, falling back to English for other languages. Emulator menus retain
upstream language options.

## Version history

Hardware-confirmed builds are in [releases/](releases/); unconfirmed builds
are in [candidates/](candidates/). Archived executables are never overwritten.

| Version | Milestone |
| --- | --- |
| [0](releases/v0/00/) | Original working catalog baseline |
| [1.00](releases/v1/00/) | NES/GBA individual XMB entries |
| [1.01](releases/v1/01/) / [1.02](releases/v1/02/) | Covers / square icons |
| [1.03](releases/v1/03/) | Full-screen backgrounds |
| [1.04](releases/v1/04/) | Separate GBA saves |
| [1.05](releases/v1/05/) | English UI and system-language selection |
| [1.06](releases/v1/06/) | Confirmed startup feedback; one-copy preparation; first-install ZIP |
| [1.07](releases/v1/07/) | GB/GBC direct launch confirmed; minimal loading bars |
| [1.08 preview](candidates/1.08/) | Atari/MD/SNES adapters and new bridge icon |

## Source, tests and licenses

```sh
git clone --recurse-submodules https://github.com/fracarli/psp-gameshelf.git
cd psp-gameshelf
git -C third_party/NesterJ-AoEX apply ../../patches/nesterj-autoload.patch
git -C third_party/TempGBA4PSP-Single-game apply ../../patches/tempgba-gameshelf.patch
git -C third_party/GBAdhoc apply ../../patches/gbadhoc-gameshelf.patch
git -C third_party/PSP2600 apply ../../patches/psp2600-gameshelf.patch
git -C third_party/Snes9xTYL apply ../../patches/snes9x-gameshelf.patch
# Install PSPDEV/PSPSDK and put its tools on PATH first.
make
python3 tools/verify_xmb.py
python3 tools/verify_gba_saves.py
python3 tools/verify_install.py
python3 tools/verify_language.py
python3 tools/verify_startup.py
python3 tools/verify_cleanup.py
python3 tools/verify_gb_launch.py
python3 tools/verify_setup.py
python3 tools/verify_extra_systems.py
python3 tools/verify_extra_install.py
python3 tools/verify_artwork.py
```

Build emulator adapters using [patches/README.md](patches/README.md).
Computer checks do not replace PSP tests. See the [roadmap](docs/roadmap.md)
and [original technical history](docs/history-v0.md).

GameShelf code uses the [MIT license](LICENSE). NesterJ, TempGBA, GBAdhoc and
PSP2600 retain upstream GPL terms. Snes9xTYL and PicoDrive retain their original
non-commercial redistribution terms and component notices. Source submodules,
adaptation patches and source/license references accompany the packages.
Only project content is published: no ROMs, BIOS, saves, downloaded artwork,
card backups or unrelated files from the developer's computer.
