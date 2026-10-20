# A single installation for several systems

The intended experience is one GameShelf download, one installation, automatic
engine selection and individual game entries in the PSP home screen. The
stable 1.08 first-install ZIP already includes the seven supported systems'
shared engines; users do not need to find each emulator themselves. Games and
the GBA BIOS are supplied by the user.

The next work should unify setup, dependency checks, updates and recovery.
A per-system manifest can describe the engine version, supported extensions,
ROM roots, save locations, required modules and file hashes. Installation
should validate the complete payload before replacement, back up affected
files and preserve user data on any failed update. Existing computer-side
installation and packaging tools provide the foundation.

On PSP, load only the selected system's runtime. The PSP-3000 has limited
memory, and existing engines have different module, graphics, audio and save
requirements. Several well-integrated cores can provide one product experience
without keeping all cores resident or forcing them into one executable.

Writing original emulation cores for every system is a separate, substantially
larger project. Existing cores already implement CPU behavior, sound, graphics,
timing, compatibility fixes and save formats. Replacing them would create a
new compatibility and validation workload. GameShelf should credit upstream
work and retain each engine's license and source obligations. Any future
single-binary design requires a separate review of those terms.

Suggested implementation sequence:

1. Validate the current complete setup ZIP on a fresh card.
2. Introduce a manifest shared by preflight, packaging and the updater.
3. Add a PSP-side setup/status flow that clearly identifies missing user files.
4. Add recoverable updates with hashes and engine/source version pinning.
5. Expand system support through adapters and hardware validation.

PSP ISO and PS1 titles continue to use native XMB launch. Minecraft and optional
Wi-Fi stores are separate homebrew applications, not GameShelf emulator cores.
