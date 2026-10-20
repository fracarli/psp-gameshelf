# GameShelf launch-request protocol

Version `v0.14.0` separates the user-mode catalog from the CFW ISO loader.

## Request file

When an ISO is selected, GameShelf creates or replaces:

`ms0:/PSP/GAMESHELF/LAUNCH.TXT`

The file contains exactly one UTF-8/ASCII line with the selected path, for
example:

```text
ms0:/ISO/Example.iso
```

The file is written atomically from GameShelf's point of view by truncating,
writing the path, adding a newline and closing the file before returning to the
UI.

## Future VSH plugin contract

The plugin must:

1. read `LAUNCH.TXT` from the Memory Stick;
2. validate that the path starts with `ms0:/ISO/` and ends in `.iso` or `.cso`;
3. mount/start the ISO through the active CFW loader;
4. remove or rename the request only after successful hand-off;
5. leave the request and report an error if the loader is unavailable.

The plugin is intentionally not included in this milestone. This keeps the
catalog homebrew safe and testable while the VSH/kernel integration is built
separately.

## N64 candidate 1.10

Mode 10 is appended without changing NES mode 9 or any previous entry IDs.
Runtime: ms0:/PSP/GAME/DaedalusX64/EBOOT.PBP. Original ROM directory:
ms0:/PSP/GAME/DaedalusX64/Roms/. The PSP loadexec argument block contains two
NUL-terminated strings: executable path, then full ROM path. Its byte count
includes both terminators. The current directory is set to the runtime root.

The root validates the basename, extension, path length and file existence.
No ROM is staged or renamed. Daedalus 1.1.8 already handles argv[1] directly;
its PSP executable is unmodified and verified against the official release.
Original emulator configuration and save behavior are retained. Hardware
compatibility and save/reload checks remain pending.
