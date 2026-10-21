# N64 hardware test notes

User-reported results, 2026-10-21. PSP-3004, firmware 6.61 PRO-C,
GameShelf 1.10 candidate, official unmodified DaedalusX64 1.1.8.
These observations are not a general compatibility guarantee. Stable 1.08
and the seven-system 1.09 installer remain unchanged.

| Game | Reported behavior | Remaining checks |
| --- | --- | --- |
| Super Mario 64 | Direct launch and smooth gameplay; initial test had audio disabled | Audio-enabled performance, save/reload, exit |
| Super Smash Bros. | Direct launch and gameplay reported without problems | Longer session, audio confirmation, save/reload, exit |
| Paper Mario | Direct launch succeeds; audio crackle and gameplay slowdowns reported | Compare identical scene with asynchronous audio and audio disabled |
| Mario Tennis | Freezes at Camelot logo | Compare launch from Daedalus menu; settings investigation |

Startup waits of approximately 17–20 seconds were reported, with one startup
around two seconds. Timing boundaries and repeatability have not been established.
Measure XMB selection to emulator handoff and first game frame separately before
attributing the delay to GameShelf. The N64 launcher passes the original ROM path;
it does not stage or copy the ROM on each launch.

## Audio experiment

Daedalus 1.1.8 defines AudioEnabled=0 as disabled, 1 as asynchronous and 2 as
synchronous. The inspected card had Paper Mario set to 2, while Super Mario 64
and Smash used 1. A backed-up, per-game change sets Paper Mario to 1 for an
A/B test, preserving all other options including AudioRateMatch=0. This is an
unconfirmed performance experiment, not a proven fix. Avoid applying one audio
configuration indiscriminately to every game. Compare the same gameplay scene
with audio enabled and disabled; record both sound quality and responsiveness.

The runtime-generated Paper Mario profile uses ID 3ae5ee653c737ded-45.
Preserve the runtime's existing profile IDs: inferring a key directly from the
big-endian ROM header can select an unused profile because PSP CRC word byte
order differs. Older unused profile sections were left intact to avoid losing
user settings; they must not be mistaken for the active profile.

## ROM replacement check

The proposed replacement Mario Tennis file on the user's Desktop is byte-identical
to the installed ROM (SHA-256 comparison). No ROM replacement was performed.
No ROM, artwork, saves or private device settings are published with these notes.

## Related PS1 observation

Crash Bandicoot 3 loads an existing save and runs according to the user, but
New Game leads to a black screen. The title, square icon and full-screen background
were corrected separately, preserving executable and PS1 payload bytes. Artwork
changes do not resolve the New Game failure. Save data must be retained.
