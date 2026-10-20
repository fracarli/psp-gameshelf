# Emulator source adaptations

These patches record the source changes used by the existing shared runtimes.
They contain source code only; the original submodules and their licenses remain
in `third_party/`. Apply each patch inside the corresponding clean submodule
at the commit recorded by the parent repository:

```
git -C third_party/NesterJ-AoEX apply ../../patches/nesterj-autoload.patch
git -C third_party/TempGBA4PSP-Single-game apply ../../patches/tempgba-gameshelf.patch
```

NesterJ reads the selected ROM at startup. TempGBA uses the fixed GameShelf
runtime directory and its internal exit menu. Build requirements remain those
of the upstream projects. Do not add local build outputs to either submodule.
