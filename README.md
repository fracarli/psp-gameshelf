# PSP GameShelf

Your retro games, directly in **Game → Memory Stick**.
Choose a game and it opens in its shared emulator without selecting the ROM again.
Square cover icons and full-screen backgrounds are supported.

**Confirmed on hardware:** NES/GBA launch, covers, backgrounds, separate GBA
saves, English UI and loading feedback. Tested on PSP-3004, 6.61 PRO-C.
**GB/GBC direct launch is also confirmed on hardware in 1.07.**
**Version 1.08 is now stable:** the user confirmed the installed setup works
on PSP. It adds Atari 2600, Mega Drive and SNES adapters. This general report
does not replace dedicated per-system save/state or fresh-card installation checks.

## First installation

You need a PSP that **already runs homebrew with custom firmware**. GameShelf
does not install or change firmware. With PRO-C, FastRecovery may be needed
after a full shutdown. Back up your memory card before copying files.

1. Download [GameShelf 1.08 setup ZIP](https://github.com/fracarli/psp-gameshelf/raw/refs/heads/main/releases/v1/08/GameShelf-1.08-setup.zip)
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
installation still needs confirmation. The user confirmed the installed 1.08
setup on the existing PSP.

| Files you provide | Location on the card |
| --- | --- |
| NES `.nes` games | `PSP/GAME/Nester_AoEX_R3/roms/` |
| GBA `.gba` games | `PSP/GAME/TempGBA4PSP-Mod/roms/` |
| GBA BIOS, named `gba_bios.bin`, 16,384 bytes | `PSP/GAME/TempGBA4PSP-Mod/` |
| GB `.gb` / GBC `.gbc` games, **1.07 and newer** | `PSP/GAME/TempGBA4PSP-Mod/roms/` |

NES and GB/GBC do not require the GBA BIOS. Scan limit: **32 games per system**,
without subfolders. ROM names determine save associations: keep names unchanged.
Run GameShelf again after adding/removing games, adding art or updating the app.
Generated entries persist after reboot.

See [START-HERE.txt](docs/START-HERE.txt) for troubleshooting and save locations.

## Single-file first installer: 1.09 candidate

[Download the 1.09 single-file setup ZIP](https://github.com/fracarli/psp-gameshelf/raw/refs/heads/main/candidates/1.09/GameShelf-1.09-single-file-setup.zip).
It contains one EBOOT to place at `PSP/GAME/GameShelfSetup/EBOOT.PBP`.
Open GameShelf Setup on an already modified PSP and press X: it extracts the
seven systems' engines, support files and ROM directories from its own package.
You supply your games and GBA BIOS, then run GameShelf to create game entries.

This candidate is for **new installations**. It preserves matching files and
stops before writes when an existing destination differs. Existing setups
continue to use the stable updater below. The installer passes computer checks
for extraction, hashes, conflicts and error rollback; PSP installation is still
unconfirmed. Stable 1.08 remains the recommended release. Read the
[1.09 instructions and recovery limits](candidates/1.09/INSTALL.txt).
The 1.09 packager reuses its archived launcher when the source has advanced.
The source on main now builds the 1.10 candidate; tag `v1.08` preserves the
stable source. The candidate also corrects the Moonptrl display name without
changing its ROM or save identity. N64 direct launch is available separately in the 1.10 candidate below.

## N64 direct launch: 1.10 candidate

The [1.10 launcher candidate](candidates/1.10/INSTALL.txt) adds individual N64
entries using the official, unmodified DaedalusX64 1.1.8 PSP runtime. Games stay
in `PSP/GAME/DaedalusX64/Roms/` (`.z64`, `.v64`, `.n64`). Covers and full-screen
backgrounds are supported. Existing runtime settings and saves remain in use.

This is a launcher-only test update for the existing setup, not a complete
first installer or a supported computer-updater target. The separate 1.09
single-file installer still includes seven systems. N64 startup, controls,
performance and saves require PSP validation before joining a stable package.
Read the candidate instructions for backups and how to show Daedalus again
if a direct launch fails.

## Updating an existing installation

Use the backup-preserving computer updater instead of merging a first-install
ZIP over an existing setup. It requires Python 3.9 or newer:

```sh
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.08 --check
python3 tools/install_release.py --sd-root /Volumes/SD-FAT32 --version 1.08 --backup-root ../PSP_BACKUP
```

Replace `/Volumes/SD-FAT32` with your card path, for example `E:/` on Windows.
Checks happen before writes. The updater backs up GameShelf and old GBA entries,
updates launcher and runtime together and verifies writes. Saves are preserved.
Open GameShelf and press **X** to regenerate entries after updating.
Restore the complete backup to roll back; do not downgrade only one executable.

## Atari, Mega Drive and SNES in 1.08

[Download the stable 1.08 setup ZIP](https://github.com/fracarli/psp-gameshelf/raw/refs/heads/main/releases/v1/08/GameShelf-1.08-setup.zip), or
use the updater above with `--version 1.08` for an existing installation.
Read the [1.08 instructions](releases/v1/08/INSTALL.txt).

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

The original bridge icon represents the connection between classic games and
the PSP home screen. The project name remains GameShelf, as chosen by its owner.

Computer checks pass, and the user confirmed the installed 1.08 setup works on
PSP. Dedicated save/state checks remain useful; the hardware report does not
provide an exhaustive test matrix for every system or game.

## Covers and backgrounds

Optional: games launch without artwork. With Python 3.10+ installed and the card
connected, run:

```sh
python3 tools/fetch_boxart.py --sd-root /Volumes/SD-FAT32 --platform nes --platform gba
# Also supported: --platform gb --platform gbc --platform atari --platform megadrive --platform snes --platform n64
```

The helper fetches available Libretro covers onto the card and prepares square
icons and darkened 480×272 backgrounds in `GAMESHELF/boxart/{nes,gba,gb,gbc,atari,megadrive,snes}/`.
Names must match the ROM basename. Rectangular covers are centrally cropped.
Run GameShelf and press X after changing artwork. Highlight a game and wait
for XMB to display its background. ROM hacks may need manually supplied art.
Downloaded art stays on the card and is never part of this repository.

## Hiding emulator entries

An optional Game Categories Lite filter can hide the shared emulator entries
while retaining their executables and all existing game/save paths. GameShelf,
FastRecovery and the N64 emulator remain visible because they are still useful.
See [the XMB filter guide](docs/xmb-filter.md). This optional configuration
is awaiting a PSP check and is not included in the stable setup ZIP.

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
| Atari / Mega Drive / SNES | Available in stable 1.08; installed setup confirmed by user |
| N64 | Direct launch implemented in 1.10 candidate; awaiting PSP confirmation |
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
| [1.08](releases/v1/08/) | User-confirmed setup; Atari/MD/SNES adapters and bridge icon |
| [1.09 candidate](candidates/1.09/) | Single-file first installer; Moon Patrol display correction; awaiting PSP test |
| [1.10 candidate](candidates/1.10/) | N64 direct launch through unmodified DaedalusX64; awaiting PSP test |

An English [presentation storyboard and narration](media/presentation/production-plan.md)
are prepared for a real-hardware demonstration.

See [the next architecture steps](docs/architecture-next.md) for a unified
installation and [optional homebrew notes](docs/optional-homebrew.md) for
Minecraft and Wi-Fi store research.

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
# To build/check the single-file installer candidate:
python3 tools/build_single_file_setup.py
python3 tools/verify_single_file_setup.py
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
