# Next steps

## Confirmed on PSP

- NES/GBA direct launch, cover icons and full-screen backgrounds.
- Independent GBA saves in 1.04.
- English UI in 1.05, with the unchanged 1.04 save runtime.

## Current candidate: 1.06

- Visible preparation/ROM-copy progress and emulator loading messages.
- One GBA ROM copy rather than two; unchanged support files are not rewritten.
- Read-only installer preflight before backup and update.
- Recoverable cleanup of verified missing-ROM entries during regeneration.

Confirm startup behavior on PSP with small and large GBA ROMs and a NES game.
Compare launch times without assuming firmware hand-off time has disappeared.
Check existing saves and test cleanup by temporarily moving, then restoring,
a ROM. Keep 1.05 as the stable rollback until these checks pass.

## Next integration

Add direct GB/GBC launch after validating 1.06. Inspect the GBAdhoc startup
interface and source before claiming selected-ROM support. Follow with Atari
as a separate adapter. Preserve a hardware-confirmed fallback for each change.

## Public demonstration

Check upstream license notices and corresponding emulator sources; prepare a
short install guide and a 30–60 second hardware demonstration showing Memory
Stick icons, backgrounds, direct launch and save recovery. Draft public posts
for review before posting to any community or social account.
