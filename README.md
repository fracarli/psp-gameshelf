# PSP GameShelf

PSP GameShelf is a lightweight homebrew library launcher for the Sony PSP.

The goal is to scan the Memory Stick, identify PSP games, PS1 titles,
homebrew and emulator libraries, then present them in one simple interface.

This is not the first PSP frontend or launcher: projects such as ARK's Custom
Launcher and newer PSP-first frontends already cover parts of this space.
GameShelf is a personal, open-source project focused on a clean setup for a
PSP 2000 with PRO-C, automatic library discovery, misplaced-file detection and
configuration that matches the owner's own Memory Stick layout.

## Target user flow

On a PSP using temporary PRO-C, the user first runs FastRecovery after a full
power-off. Then the user opens `Game > Memory Stick` and starts `PSP GameShelf`.
GameShelf becomes the unified library screen: it scans `ms0:/ISO` for PSP
games, `ms0:/PSP/GAME` for PS1 and homebrew, and configured ROM folders for
other systems. The user sees all supported titles in one catalog instead of
opening each emulator separately.

GameShelf will use existing emulators and launch them with the selected ROM;
it does not redistribute games, ROMs, BIOS files or emulator binaries.

## Planned features

- Scan `ms0:/ISO` for PSP games.
- Scan `ms0:/PSP/GAME` for PS1 and homebrew applications.
- Detect Game Boy, GBA, NES, Atari 2600, Mega Drive and N64 libraries.
- Group entries by platform.
- Mark favourites and recently played entries.
- Detect misplaced files and duplicate names.
- Launch PSP games and the correct emulator.
- Keep configuration and favourites on the Memory Stick.

## Current status

Version `v0.51.0` is the latest confirmed stable build on real hardware:
PSP-3004, TA-095v2, firmware 6.61 with PRO-C. It provides a read-only scanner
for PSP ISO/CSO files and folders under `PSP/GAME`, plus basic navigation and
selection. Versions `v0.11.0` through `v0.13.0` tested direct ISO launch calls,
but PRO-C returns `80010002`: a standalone user-mode EBOOT cannot hand an ISO
to the CFW loader as if it were a normal executable.

Version `v0.14.0` introduces the launch-request protocol. Selecting an ISO
writes its path to `ms0:/PSP/GAMESHELF/LAUNCH.TXT`; a future VSH plugin will
consume that request and call the CFW ISO loader from the correct context.

The VSH plugin is still a separate milestone. Until it exists, GameShelf
remains a safe catalog and ISO selection only creates the request file; the
XMB/CFW launcher remains responsible for starting ISO files.

The v0.15 release adds the first experimental VSH plugin at
`plugin/vsh/gameshelf_vsh.prx`. It is compiled but not yet hardware-verified
and must remain disabled in `seplugins/vsh.txt` until the plugin test is ready.

Version `v0.17.0` removes the previous 55-character filename limit. Full ISO
filenames are now preserved when writing `LAUNCH.TXT`, including long names
with region and language metadata.

Version `v0.18.0` adds alphabetical ordering for ISO/CSO and homebrew entries,
keeps the selection cursor valid after a refresh, and updates the ISO status
message for the normal plugin-disabled workflow. It has been confirmed on the
target PSP hardware; v0.16 remains the fallback if needed.

Version `v0.19.0` adds category navigation for PSP ISO/CSO, PlayStation 1,
homebrew, Game Boy/Game Boy Advance, NES, Atari 2600, Mega Drive/Genesis and
Nintendo 64. Left/Right changes category and Up/Down changes the selected
entry. ROM entries are catalogued safely but are not launched automatically;
the corresponding emulator remains responsible for starting them.

Version `v0.20.0` hardens the category scan for PRO-C by avoiding nested
directory reads during startup. This is the corrected build to test after the
black-screen report from v0.19.

Version `v0.21.0` is a safe recovery build after v0.20 still showed a black
screen on the target PSP. Optional emulator ROM-folder scanning is disabled;
the build keeps the stable PSP, PS1 and homebrew catalog paths while the ROM
scanner is redesigned for better PRO-C compatibility.

Version `v0.22.0` restores the proven v0.18 architecture exactly: one simple
scan of `ISO` and `PSP/GAME`, alphabetical ordering, and no experimental
category or emulator-folder code. It is the new baseline after the category
builds produced a black screen on hardware.

Version `v0.23.0` adds two safe in-memory views over that same scan: PSP
ISO/CSO and Applications/Homebrew. Left/Right switches view, while the
filesystem scan remains unchanged from v0.22. This is the first incremental
feature after the recovery build.

Version `v0.24.0` adds the first optional emulator library: Game Boy Advance
ROMs in `PSP/GAME/TempGBA4PSP-Mod/roms`. The folder is scanned only when the
GBA view is opened, not during startup. Selecting a ROM currently identifies
it and points to the installed emulator; automatic ROM hand-off will be added
only after this catalog view is confirmed stable.

Version `v0.25.0` hardens the GBA view after a hardware freeze: it accepts only
`.gba`, `.gb` and `.gbc` files, and disables the Triangle refresh action while
the GBA view is active. PSP and Homebrew refresh behavior remains unchanged.

Version `v0.26.0` disables manual refresh in every view. Hardware testing
showed that pressing Triangle to rescan `ISO` or `PSP/GAME` could freeze the
PSP even though the initial scan worked. The catalog is now refreshed at
startup only; Triangle displays an informational message and performs no file
system operation.

Version `v0.27.0` adds the optional NES view from
`PSP/GAME/Nester_AoEX_R3/roms`. It uses the same lazy-scan and no-refresh rules
as the GBA view. NES ROMs are catalogued but still start through Nester.

Hardware testing confirmed v0.27.0 stable: the NES view opens and displays the
ROMs correctly on the target PSP. The next milestone is testing the launch of
the installed Nester emulator itself before attempting automatic ROM hand-off.

Version `v0.28.0` adds the first emulator launch adapter: pressing X in the NES
view starts `PSP/GAME/Nester_AoEX_R3/EBOOT.PBP`. It deliberately does not pass
a ROM argument yet; the first test verifies only that GameShelf can launch the
installed emulator safely.

Version `v0.29.0` adds a custom PSP icon at `assets/ICON0.PNG`, embedded into
the EBOOT so GameShelf is recognizable in the Memory Stick menu instead of
using the default grey icon.

Version `v0.30.0` passes the selected NES ROM path to Nester when launching the
emulator. Nester's included documentation does not guarantee command-line ROM
loading, so this is an experimental compatibility test: it may open the ROM
directly or fall back to Nester's normal menu.

Hardware testing showed the exact behavior: v0.30 starts Nester in the correct
ROM folder, but Nester still waits for the user to select the game. The next
step is an emulator-side adapter: use the available NesterJ source to read the
startup argument and call its internal ROM-load routine. GameShelf already
provides the correct path; the missing behavior belongs inside Nester.

The first patched NesterJ build is now available at
`releases/nesterj-autoload-v1/EBOOT.PBP`. It preserves the original emulator
folder and adds startup ROM loading for `.nes`, `.fds`, `.fam`, `.unf`, `.nsf`
and `.zip` arguments. This is a hardware-test build, not yet a numbered
GameShelf release: back up the existing EBOOT and copy this one over
`PSP/GAME/Nester_AoEX_R3/EBOOT.PBP`, then use GameShelf v0.30 to launch a NES
ROM. If the ROM opens directly, the next GameShelf version will target this
adapter; if it does not, the PSP screen/result will guide the next fix.

Hardware testing confirmed that the patched NesterJ build starts the selected
NES ROM directly. GameShelf `v0.31.0` is the first release after that
milestone; its EBOOT is archived at `releases/v0/31/EBOOT.PBP`. The patched
NesterJ EBOOT must remain installed in
`PSP/GAME/Nester_AoEX_R3/EBOOT.PBP` for direct NES launch to work.

Version `v0.32.0` is the next hardware-test candidate. It adds the first GBA
launch adapter: selecting a `.gba`, `.gb` or `.gbc` file passes its full path
to `PSP/GAME/TempGBA4PSP-Mod/EBOOT.PBP`. The candidate is archived at
`releases/v0/32/EBOOT.PBP`; v0.31 remains the stable fallback until GBA launch
is tested on hardware.

Version `v0.33.0` corrects the GBA hand-off after testing showed that TempGBA
does not accept the ROM path as a startup argument. GameShelf now starts
TempGBA without an argument, preserving its own `dir.ini`, BIOS and ROM-folder
context. The build is archived at `releases/v0/33/EBOOT.PBP`; it should open
TempGBA reliably, but the user still selects the ROM inside TempGBA until a
TempGBA-specific autoload adapter is implemented.

Version `v0.34.0` tests the public TempGBA Single-game fork, which accepts a
ROM path at startup. GameShelf points the GBA adapter to
`PSP/GAME/TempGBA-Single-Game/EBOOT.PBP` and passes the selected `.gba` path.
The test package is archived at `releases/v0/34/EBOOT.PBP`; the separate
emulator package is staged under the user's PSP folder and does not replace
the known-working `TempGBA4PSP-Mod` installation. Its `dir.ini` points to the
existing `TempGBA4PSP-Mod/roms` folder, so the two emulator folders do not
require duplicate ROM copies.

Version `v0.35.0` changes the GBA hand-off again after TempGBA rejected both
the external ROM directory and the startup context. GameShelf now copies the
selected `.gba` temporarily to
`PSP/GAME/TempGBA-Single-Game/roms/game.gba` and launches the emulator without
an argument. This keeps the emulator's BIOS and directory checks local while
avoiding a permanent duplicate library. Only `.gba` files are shown in the
GBA view. The test build is archived at `releases/v0/35/EBOOT.PBP`.

Version `v0.36.0` corrects the launch context. GameShelf now changes the
working directory to `TempGBA-Single-Game` before launching GBA and passes no
argument, so TempGBA can use its local `dir.ini` and staged `roms/game.gba`.
The NES launch path also sets NesterJ's working directory explicitly while
preserving the selected-ROM argument.

Version `v0.37.0` is a stability candidate after a hardware freeze while
switching from the GBA view to NES. It initializes directory entries before
each scan and limits the NES catalog to `.nes` ROMs, avoiding unexpected files
inside the emulator directory. GBA launching and the v0.36 working-directory
fix are otherwise unchanged.

Version `v0.38.0` adds scrolling to all catalog views. When a platform has more
than 18 entries, Up and Down move through the complete list and the current
range is shown on screen. Emulator launch paths are unchanged from v0.37.

Version `v0.39.0` switches the GBA launch path back to the installed
`TempGBA4PSP-Mod` emulator after hardware testing confirmed that it works while
the Single-Game fork produced a black screen. Selecting a GBA title now opens
the known-working emulator interface without staging files or passing startup
arguments. Automatic ROM selection remains a separate future adapter task.

Version `v0.40.0` adds a GBA runtime bridge. Before launching TempGBA-Mod,
GameShelf writes a temporary `dir.ini` with absolute Memory Stick paths and
stages the BIOS and support modules in the GameShelf launch directory. This
addresses the case where TempGBA works when started directly but cannot find
`dir.ini` when launched from another homebrew.

Version `v0.41.0` extends the bridge by copying TempGBA-Mod's EBOOT into a
temporary `PSP/GAME/GAMESHELF/TempGBA/` launch folder. The emulator, BIOS,
configuration and support modules now share the same launch context instead of
depending on the PSP loader's inherited working directory.

Version `v0.42.0` removes the absolute paths from that temporary configuration.
It creates the local TempGBA directories, writes a relative `dir.ini`, and
copies the selected `.gba` to `TempGBA/roms/game.gba` before launching. This
matches the directory model used by the standalone emulator.

Version `v0.43.0` accounts for TempGBA's use of the inherited current working
directory. It also stages the local `dir.ini`, BIOS context and selected ROM
directly under `PSP/GAME/GAMESHELF/`, which is the directory inherited when the
emulator is launched from the shelf.

Version `v0.44.0` is a stability and diagnostics release. GBA selection no
longer launches the external emulator. Pressing `X` in the GBA view checks the
known ROM, BIOS and configuration paths, writes `GBA_DIAG.TXT` under
`PSP/GAME/GAMESHELF/`, and displays a confirmation on screen. This avoids the
crash/error loop while collecting the information needed for an integrated GBA
core.

Version `v0.45.0` extends the diagnostic log with GameShelf's actual current
working directory. TempGBA uses the same concept internally, so this value is
needed to implement the final launch adapter without guessing the PSP loader's
behavior.

Version `v0.46.0` re-enables the GBA launch using the verified working directory
and local runtime files from the v0.45 diagnostic. It stages the selected ROM,
BIOS and relative `dir.ini` under `GAMESHELF`, then launches the copied TempGBA
EBOOT. If preparation fails, it falls back to `GBA_DIAG.TXT` instead of
blocking the PSP.

Version `v0.47.0` is the first source-level TempGBA integration build. The
TempGBA EBOOT is rebuilt from the included source with a deterministic runtime
path (`PSP/GAME/GAMESHELF/TempGBA/`) instead of trusting the current working
directory inherited from the PSP loader. GameShelf also preserves this
integration EBOOT when preparing a ROM, so it no longer replaces it with the
standalone TempGBA EBOOT. This release still requires hardware testing; the
expected result is that selecting a GBA ROM reaches the TempGBA interface
without the previous `dir.ini`/BIOS path errors.

Version `v0.48.0` hardens the TempGBA configuration by writing absolute PSP
paths for every directory entry. This addresses the remaining `specified
directory is invalid` errors observed when the PSP loader resolves relative
paths differently during a GameShelf hand-off.

Version `v0.49.0` removes the legacy directory validation step from the
GameShelf-integrated TempGBA build. TempGBA now assigns its runtime folders
directly in memory, avoiding the old `sceIoDopen()` checks that continued to
report false `specified directory is invalid` errors even when the folders
were present on the card.

Version `v0.51.0` disables the firmware HOME popup for the integrated TempGBA
runtime because pressing HOME after a cross-homebrew launch can crash the PSP.
TempGBA's own menu remains available through its controller mapping and should
be used for `Quit/Exit` until a safe return-to-GameShelf hand-off is implemented.

Version `v0.52.0` adds platform badges to the catalog views and creates the
cover-art directory structure on the Memory Stick:
`PSP/GAME/GAMESHELF/boxart/{psp,homebrew,gba,nes}/`. This is the first visual
metadata step; actual image loading and additional emulator platforms follow in
later releases.

Version `v0.53.0` recognizes user-provided PNG covers. A cover uses the game
filename without its extension, for example `FIFA 13.png`, and is placed in
the matching platform folder. The catalog reports whether the selected entry
has a matching cover; emulator launch behavior is unchanged.

Version `v0.54.0` adds the Mac helper `tools/fetch_boxart.py`. It scans the
connected SD card, downloads matching PNG box art from the public Libretro
thumbnail index, and creates a PSP-native `.gsh` texture beside each PNG.
GameShelf renders the `.gsh` texture directly in the catalog, avoiding a PNG
decoder in the PSP EBOOT. It never uploads ROMs, BIOS files or downloaded
covers to this repository.

Version `v0.55.0` renders the generated `.gsh` textures in the catalog and
supports the framebuffer pixel formats used by PSP models. The v0.53 EBOOT
remains the recovery version if a hardware-specific display issue occurs.

Version `v0.56.0` adds Game Boy and Game Boy Color catalog views. These ROMs
are handed to the existing GBAdhoc 4.1.0 homebrew runtime, while NES and GBA
launch paths remain unchanged.

Version `v0.57.0` adds catalog views and emulator hand-off paths for Atari 2600,
Mega Drive and Super Nintendo using the existing `psp2600`, `PicoDrive` and
`s9xTYLme_mod` installations.

Version `v0.58.0` adds automatic PS1 folder detection and a dedicated PS1 view.
Folders containing regional PlayStation identifiers such as `SCUS`, `SLUS`,
`SLES` or `SCES` are launched through their existing `EBOOT.PBP`.

Version `v0.59.0` verifies the selected ISO before creating the launch request
and writes `PSP/GAMESHELF/ISO_STATUS.TXT` with the exact hand-off result. This
keeps the ISO path recoverable while the PRO-C-specific VSH loader remains
experimental.

Version `v0.60.0` fixes PS1 launching by matching the XMB behavior: POPS
EBOOTs are loaded without passing a ROM path as an argument.

Version `v0.62.0` uses the dedicated PS1 Memory Stick API type
(`PSP_INIT_APITYPE_MS_PS1`) and sets the CFW init key to POPS
(`PSP_INIT_KEYCONFIG_POPS`) before launching a PS1 EBOOT. It also records both
values in `PSP/GAMESHELF/PS1_STATUS.TXT` for hardware diagnostics.

Version `v0.63.0` backs off to the regular `PSP_INIT_APITYPE_MS2` after the
target PRO-C returned `0x80020001` with `MS_PS1`; it keeps the POPS key
configuration while using the API that the previous launcher accepted.

Hardware testing confirmed v0.51.0 stable on the target PSP for NES and GBA:
the catalog loads, NES ROMs launch through NesterJ, GBA ROMs launch through the
integrated TempGBA runtime, and GBA games can exit through TempGBA's internal
Quit/Exit menu without using the crashing firmware HOME popup.

The v0.17 VSH experiment is not a working direct-launch solution on the target
PRO-C installation. The plugin loads, reads the complete request path and
reaches the mount call, but PRO-C returns `0x8002013A`
(`SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED`) when GameShelf calls the public
system-control mount function. This confirms that the problem is the runtime
integration point, not the ISO filename or its folder. v0.17 must therefore be
considered experimental and should remain disabled for normal use.

Hardware testing also confirmed that the same ISO files launch normally from
the standard PSP `Memory Stick` menu with the plugin disabled. The games and
the card layout are therefore working correctly; only the custom VSH mount
integration remains unresolved.

The next implementation milestone is a supported hand-off to an existing
CFW-compatible launcher (or an ARK-based launcher), while GameShelf remains
the catalog and selection interface. GameShelf will not attempt to mount an
ISO through an unavailable PRO-C library function.

See [the launcher integration plan](docs/launcher-integration-plan.md) for the
current architecture and test criteria.

### Hardware test target

The first hardware target is a PSP-3004 with TA-095v2 motherboard, firmware
6.61 and PRO-C. The build explicitly targets firmware 6.61.

## Build naming

Installed PSP applications must always keep the filename `EBOOT.PBP` inside
their GameShelf folder. Archived builds are separated by version directories:
`releases/v0/00/EBOOT.PBP`, `releases/v0/01/EBOOT.PBP`, then `02`, `03` and so
on. When the project reaches the next major version, the structure becomes
`releases/v1/00/EBOOT.PBP`. The folder identifies the build; the filename stays
compatible with the PSP.

## Building

The project requires the PSPDEV/PSPSDK toolchain. Once installed:

```sh
git clone --recurse-submodules https://github.com/fracarli/psp-gameshelf.git
cd psp-gameshelf
make
```

With the PSP SD card mounted as `SD-FAT32`, fetch covers with:

```sh
./tools/fetch_boxart.py
```

Use `--sd-root /path/to/card` for another mount point and `--force` to
redownload existing images.

The two emulator sources under `third_party/` are tracked as Git submodules
so their original repositories and licenses remain clear. Use
`git submodule update --init --recursive` when cloning without the recurse
option. This repository does not include ROMs, save data or BIOS files; users
must provide legally owned files separately.

The resulting `EBOOT.PBP` can be copied to `PSP/GAME/GAMESHELF/` on a PSP
running custom firmware. This repository contains no games, ROMs or BIOS files.

## License

Code: MIT. GameShelf is intended to manage software owned by the user and
legally distributed homebrew.
# psp-gameshelf
