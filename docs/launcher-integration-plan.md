# Launcher integration plan

## Current finding

GameShelf v0.17 successfully writes and reads a complete ISO request from
`ms0:/PSP/GAMESHELF/LAUNCH.TXT`. The experimental VSH plugin also loads on the
target PSP-3004, but `sctrlSEMountUmdFromFile()` returns `0x8002013A` under the
tested PRO-C installation. The direct mount path is therefore not a supported
runtime boundary for this setup.

## Chosen architecture

GameShelf remains responsible for:

- scanning and grouping the user's installed titles;
- showing metadata and the selected platform;
- validating that a selected path exists;
- recording a launch request for a compatible launcher.

An existing CFW-compatible launcher remains responsible for mounting and
starting PSP ISO files. GameShelf will not redistribute games, ROMs, BIOS files
or proprietary launcher binaries.

## Implementation milestones

1. Keep the v0.16/v0.17 catalog and request protocol stable.
2. Define a launcher adapter interface, beginning with PSP ISO hand-off.
3. Test the adapter with the user's installed PRO-C setup before changing the
   VSH plugin again.
4. Add emulator-specific adapters only after PSP ISO launching is reliable.
5. Keep the experimental VSH plugin disabled unless a compatible runtime API is
   confirmed.

## Acceptance criteria for the next version

- GameShelf still starts without a black screen.
- PSP ISO entries display their complete filenames.
- Selecting an ISO produces a clear success or failure status.
- The existing CFW launcher starts the selected ISO without manually editing
  files for each game.
- Failure leaves the PSP recoverable and does not require deleting the ISO.
