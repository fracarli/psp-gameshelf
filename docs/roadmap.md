# Roadmap

## v0.1 — foundation

- PSP SDK project skeleton
- startup screen
- clean repository layout

## v0.2 — scanner

- scan PSP ISO files
- scan `PSP/GAME`
- detect platform from paths and extensions
- display a scrollable list

## v0.3 — emulator routing

- map each platform to an emulator
- launch homebrew and PS1 entries
- add user-editable configuration

## v0.4 — library experience

- categories
- favourites
- recently played
- cover art and metadata
- free-space indicator

## Current status

v0.51 is stable on the target PSP for NES and GBA. NES uses the patched
NesterJ launcher and GBA uses the source-integrated TempGBA runtime. The
firmware HOME popup is intentionally disabled for GBA; the emulator's own
Quit/Exit menu is the safe exit path.

## Current development sequence

1. Improve the v0.54 cover layout and replace text badges with bitmap icons
   where supported.
2. Add Atari, Mega Drive and SNES adapters one at a time, preserving a stable
   fallback for every failure. Game Boy and Game Boy Color are covered by
   v0.56 through GBAdhoc.
3. Add a safe return from TempGBA to GameShelf.
4. Add favourites, recently played entries and free-space information.
5. Add an installer/checker that validates emulator files, BIOS files and
   directory structure.
6. Prepare the v1.0 release package, documentation, screenshots and demo.

## v1.0 — release

- safe read-only diagnostics
- polished theme
- release package
- screenshots and demo video
- English documentation
