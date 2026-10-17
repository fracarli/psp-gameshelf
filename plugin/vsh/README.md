# GameShelf VSH plugin

This is the experimental VSH-side component for GameShelf v0.17. It watches
`ms0:/PSP/GAMESHELF/LAUNCH.TXT` and validates an ISO/CSO path under
`ms0:/ISO/`.

On the target PSP-3004 with firmware 6.61 and PRO-C, the plugin loads and
receives the full path correctly, but the PRO system-control mount call returns
`0x8002013A` (`SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED`). It therefore does not
currently mount or launch games and must be treated as a diagnostic prototype,
not as the normal GameShelf launcher.

The plugin is not enabled automatically. After a successful build, copy
`gameshelf_vsh.prx` to `ms0:/seplugins/` and add this line to `ms0:/seplugins/vsh.txt`:

```text
ms0:/seplugins/gameshelf_vsh.prx 1
```

To disable it, change the final `1` to `0` or remove the line. Reset VSH after
changing the file. Keep the confirmed catalog EBOOT available as a fallback.

The supported next step is to integrate GameShelf with an existing CFW/ARK
launcher instead of calling the unavailable mount function directly. See
`../../docs/launcher-integration-plan.md`.
