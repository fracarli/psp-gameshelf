# Architecture

## First milestone

The first milestone deliberately separates discovery from launching:

1. `scanner`: reads known Memory Stick locations and produces library entries;
2. `catalog`: normalizes names and assigns a platform;
3. `ui`: renders categories and the selected entry;
4. `launcher`: starts a PSP ISO, PS1 EBOOT or emulator;
5. `settings`: stores favourites and user-defined emulator paths.

The scanner must be read-only. Any cleanup operation will require an explicit
confirmation and will never delete a file automatically.

## Known library locations

| Platform | Location | Typical extension |
|---|---|---|
| PSP | `ms0:/ISO` | `.iso`, `.cso` |
| PS1/homebrew | `ms0:/PSP/GAME/*` | `EBOOT.PBP` |
| GB/GBC/GBA | emulator-specific `roms` folder | `.gb`, `.gbc`, `.gba` |
| NES | NesterJ `roms` folder | `.nes` |
| Atari 2600 | PSP2600 `roms` folder | `.a26`, `.bin` |
| Mega Drive | PicoDrive `roms` folder | `.md`, `.bin` |
| N64 | DaedalusX64 `Roms` folder | `.z64`, `.v64`, `.n64` |

