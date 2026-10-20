#!/usr/bin/env python3
"""Install a GameShelf main EBOOT after backing up the existing application.

This is an update helper: emulator runtimes must already be installed.
ROMs, BIOS, save data and generated game entries are left in place.
"""
import argparse
from datetime import datetime
import hashlib
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sd-root', required=True, type=Path)
    parser.add_argument('--version', choices=('1.00', '1.01', '1.02', '1.03'), default='1.03')
    parser.add_argument('--backup-root', required=True, type=Path)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    package = project / ('candidates/1.03' if args.version == '1.03' else 'releases/v1/' + args.version.split('.')[1])
    source = package / 'EBOOT.PBP'
    for line in (package / 'SHA256SUMS.txt').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if hashlib.sha256((package / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError('Package checksum mismatch: ' + name)
    target = args.sd_root / 'PSP/GAME/GAMESHELF/EBOOT.PBP'
    if not target.is_file():
        raise RuntimeError('Existing GameShelf installation not found')
    backup = args.backup_root / ('before_' + args.version + '_' + datetime.now().strftime('%Y%m%d_%H%M%S'))
    shutil.copytree(target.parent, backup / 'GAMESHELF')
    print('Backup:', backup, flush=True)
    temp = target.with_name('GSH_UPDATE.TMP')
    expected = hashlib.sha256(source.read_bytes()).digest()
    shutil.copyfile(source, temp)
    if hashlib.sha256(temp.read_bytes()).digest() != expected:
        raise RuntimeError('SD write verification failed; original EBOOT retained')
    temp.replace(target)
    if hashlib.sha256(target.read_bytes()).digest() != expected:
        raise RuntimeError('Installed EBOOT verification failed')
    print('Installed and verified GameShelf', args.version)
    print('On PSP, open GameShelf and press X to update the Memory Stick entries.')


if __name__ == '__main__':
    main()
