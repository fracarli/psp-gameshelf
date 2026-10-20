# Emulator source adaptations

These patches record the source changes used by the existing shared runtimes.
They contain source code only; the original submodules and their licenses remain
in `third_party/`. Apply each patch inside the corresponding clean submodule
at the commit recorded by the parent repository:

```
git -C third_party/NesterJ-AoEX apply ../../patches/nesterj-autoload.patch
git -C third_party/TempGBA4PSP-Single-game apply ../../patches/tempgba-gameshelf.patch
```

NesterJ reads the selected ROM at startup. TempGBA uses the fixed GameShelf
runtime directory and its internal exit menu. Build requirements remain those
of the upstream projects. Do not add local build outputs to either submodule.

The 1.04 runtime includes `shared/gba_identity.h` from the parent repository.
It uses the original ROM name for save/state/config files and rejects missing
or inconsistent staged-ROM identities. Keep the submodule in its recorded
parent-repository location when building.

## GB/GBC adapter (1.07 preview)

```
git -C third_party/GBAdhoc apply ../../patches/gbadhoc-gameshelf.patch
python3 tools/build_gb_runtime.py
```

The helper builds the core with upstream `CORE_COMMON` flags and the release
profile, then the native PSP frontend with `GPSP_PLAYABLE`, `VID_TRIPLE`,
`GPSP_PROFILE_RELEASE=1` and `ME_CATCH=1`. This dedicated GB/GBC build does not
enable the optional GBA link or rewind prototypes. Source: GBAdhoc commit
`205b7cc9113cb8dc6160fc91ab6330df757d1955` (recorded submodule).

The adapter accepts three PSP arguments: executable path, `--gameshelf`, and
an absolute GB/GBC ROM path in the GameShelf library. It reads the ROM in place,
selects the console from its extension and retains normal console-specific
save/state paths. Invalid or missing requests exit without choosing another ROM.
An external-launch loading bar replaces detailed startup text.

For the exact 1.06 TempGBA source adaptation, use the patch archived under
`releases/v1/06/tempgba-gameshelf.patch`; the current patch includes 1.07 bars.


## Atari 2600 and SNES adapters (1.08)

```sh
git -C third_party/PSP2600 apply ../../patches/psp2600-gameshelf.patch
git -C third_party/Snes9xTYL apply ../../patches/snes9x-gameshelf.patch
make -C third_party/PSP2600/src -f Makefile-3x
make -C third_party/Snes9xTYL mehomeadhoc
make -C third_party/Snes9xTYL/psp/mediaengineprx
make -C third_party/Snes9xTYL/psp/homehookprx
```

Both adapters include `shared/extra_launch.h` from the parent project. They
accept executable and original ROM path as two NUL-separated PSP arguments.
The Atari adapter retains the original PSP2600 data directory and per-game
state/settings names, skipping its splash and initial menu during direct launch.
Its build disables the optional infrared keyboard and reports it unavailable
to the SDK SDL library; ordinary controller input remains enabled.

SNES retains `s9xTYLme_mod` as its data/module/save directory and skips welcome
and selection for explicit requests. The patches include SDK type/prototype,
C dialect and zlib linkage updates needed for the current PSPDEV toolchain.
They retain emulation code and original notices. The setup does not copy the
upstream music archive, which contains game audio.

PicoDrive is the unmodified official PSP 2.05 binary. Its native ROM argument
interface needs no core patch. The archive provenance, checksum, source tag
and original license are recorded under `releases/v1/08/PicoDrive/`.
