# Next steps

1. Validate the 1.09 single-file first installer on a fresh card: extraction,
   game generation, correct engine selection and save/reload on PSP. Stable
   1.08 remains the fallback; the candidate does not replace differing files.
2. Add N64 direct launch after a user-supplied ROM is available. Upstream
   DaedalusX64 PSP code accepts a ROM argument, but the installed binary and
   selected game's compatibility must be checked before hiding DaedalusX64.
3. Extend setup to recover after forced exit/power interruption and perform
   backup-preserving updates of existing installations.
4. Add a PSP-side setup/status flow for missing user files, especially the
   GBA BIOS, and guide ROM placement without exposing internal engine choices.
5. Complete per-system save/state and missing-ROM cleanup checks, then prepare
   the English public demonstration and LinkedIn project update.

The user confirmed the installed 1.08 setup and Minecraft run on PSP-3004,
6.61 PRO-C. Tag v1.08 preserves that stable project source. A private checkpoint
backs up the launcher, runtime settings and saves. Fresh-card installer 1.09
and its launcher changes remain unconfirmed on hardware. PSP ISO/PS1 use native
XMB launch. The project name remains GameShelf.
