#!/usr/bin/env python3
"""Read-only checks for an existing PSP GameShelf setup. BIOS is never supplied."""
import argparse
import struct
from pathlib import Path


def check_installation(sd_root, package):
    game = sd_root / 'PSP/GAME'
    issues = []
    def required(relative, kind='file', size=None, pbp=False):
        path = game / relative
        if kind == 'dir':
            if not path.is_dir(): issues.append(f'Missing directory: PSP/GAME/{relative}')
            return
        if not path.is_file():
            issues.append(f'Missing file: PSP/GAME/{relative}')
            return
        if path.stat().st_size == 0 or (size is not None and path.stat().st_size != size):
            issues.append(f'Unexpected file size: PSP/GAME/{relative}' + (f' (expected {size} bytes)' if size else ''))
        if pbp:
            with path.open('rb') as stream:
                header = stream.read(40)
            valid = len(header) == 40 and header[:4] == b'\0PBP'
            if valid:
                offsets = struct.unpack_from('<8I', header, 8)
                valid = offsets[0] >= 40 and list(offsets) == sorted(offsets) and offsets[-1] <= path.stat().st_size and offsets[6] < offsets[7]
            if not valid: issues.append(f'Invalid PBP header: PSP/GAME/{relative}')
    required('GAMESHELF/EBOOT.PBP', pbp=True)
    required('GAMESHELF/TempGBA/EBOOT.PBP', pbp=True)
    required('TempGBA4PSP-Mod/roms', 'dir')
    required('TempGBA4PSP-Mod/gba_bios.bin', size=16384)
    for name in ('tempgba_me.prx', 'cp_fix_gbk.prx', 'exception.prx'):
        required('TempGBA4PSP-Mod/' + name)
    if not (package / 'TempGBA/ku_bridge.prx').is_file():
        required('GAMESHELF/TempGBA/ku_bridge.prx')
    required('Nester_AoEX_R3/EBOOT.PBP', pbp=True)
    required('Nester_AoEX_R3/roms', 'dir')
    return issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sd-root', type=Path, required=True)
    args = parser.parse_args()
    package = Path(__file__).resolve().parents[1] / 'releases/v1/06'
    issues = check_installation(args.sd_root, package)
    for issue in issues: print('ERROR:', issue)
    if not issues: print('PASS: required folders, executables, support modules and BIOS size. No files changed.')
    return 2 if issues else 0


if __name__ == '__main__':
    raise SystemExit(main())
