# Optional homebrew applications

These applications are separate from the GameShelf release payload. Their
binaries, third-party assets and personal data are not committed here.

## Minecraft-PE-PSP

The [upstream project](https://github.com/Pencea-Flavius/Minecraft-PE-PSP)
provides a PSP port based on the early Pocket Edition game. Its README describes
support for every PSP model and saving worlds beside the executable.

Version [1.9.9](https://github.com/Pencea-Flavius/Minecraft-PE-PSP/releases/tag/v1.9.9)
was copied to PSP/GAME/MCPSP on the existing card. EBOOT.PBP and data/ must stay
together. The archive contains 94,405,076 bytes before filesystem overhead;
the download is 79,517,361 bytes. Copy verification passed. Running, controls
and world save/reload still need a user hardware test.

Archive SHA256: 575076729de3784fb002712e84453d583ec1a2bf88c75236c3ce940479ce7283

Worlds are saved in PSP/GAME/MCPSP/saves/. Back up that directory before future
updates. The original release and a file-hash installation record are kept in
a private computer backup. The upstream project documents separate rights for
its original PSP engine work and Mojang-derived code/assets; this repository
does not redistribute the Minecraft package.

## Downloads over Wi-Fi

The user's unnamed application cannot be identified conclusively from its
description. These are relevant options:

- [PKGi PSP](https://github.com/bucanero/pkgi-psp) supports configurable catalogs,
  games, themes, demos, applications and emulators. Its catalog determines what
  is available; installing the client does not supply all of those catalogs.
  The [PSPDEV homebrew database](https://github.com/pspdev/homebrew) publishes
  a PKGi-compatible homebrew catalog.
- [FREESTOR](https://github.com/GorGylka/FREESTOR) is declared no longer updated
  by its author. Its README also lists a conflict with category_lite.prx, which
  matters when using the optional emulator filter.
- [StoreStation](https://github.com/StoreStation/StoreStation) documents ARK4
  compatibility and ARK's WPA2 option. That does not establish compatibility
  with this project's tested 6.61 PRO-C configuration.

A Wi-Fi store is useful for small homebrew and themes if a compatible connection
is available. For larger transfers and backups the card reader remains useful.
Check Wi-Fi compatibility on the actual firmware before choosing a client.
Commercial content may require valid activation/licensing, as PKGi documents.
No store, catalog, activation plugin or firmware change was installed as part
of the Minecraft and GameShelf update.
