#!/usr/bin/env python3
"""Update GameShelf, backing up installed runtimes and personal data first."""
import argparse
from datetime import datetime
import hashlib
from pathlib import Path
import shutil
import struct
from check_installation import check_installation


def digest(path):
    return hashlib.sha256(path.read_bytes()).digest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sd-root', required=True, type=Path)
    parser.add_argument('--version', choices=('1.00', '1.01', '1.02', '1.03', '1.04', '1.05', '1.06', '1.07', '1.08'), default='1.08')
    parser.add_argument('--backup-root', type=Path)
    parser.add_argument('--check', action='store_true', help='check prerequisites without writing to the card')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    package = project / ('candidates/' + args.version)
    if not package.is_dir(): package = project / ('releases/v1/' + args.version.split('.')[1])
    for line in (package / 'SHA256SUMS.txt').read_text().splitlines():
        expected, name = line.split('  ', 1)
        if hashlib.sha256((package / name).read_bytes()).hexdigest() != expected:
            raise RuntimeError('Package checksum mismatch: ' + name)
    issues = check_installation(args.sd_root, package)
    if issues:
        raise RuntimeError('Installation checks failed before any write:\n' + '\n'.join(issues))
    if args.check:
        print('PASS: installation checks and package hashes. No files changed.')
        return
    if args.backup_root is None:
        parser.error('--backup-root is required unless --check is used')
    game = args.sd_root / 'PSP/GAME'
    shelf = game / 'GAMESHELF'
    target = shelf / 'EBOOT.PBP'
    version_file = shelf / 'INSTALL_VERSION.TXT'
    if not target.is_file():
        raise RuntimeError('Existing GameShelf installation not found')
    if version_file.exists() and version_file.read_text().strip() in ('1.04', '1.05', '1.06', '1.07', '1.08') and args.version not in ('1.04', '1.05', '1.06', '1.07', '1.08'):
        raise RuntimeError('To roll back the save-isolation runtime, restore the complete backup, including TempGBA and generated entries')
    updates = [(package / 'EBOOT.PBP', target)]
    entries = []
    if args.version in ('1.04', '1.05', '1.06', '1.07', '1.08'):
        runtime = shelf / 'TempGBA/EBOOT.PBP'
        if not runtime.is_file():
            raise RuntimeError('Existing integrated TempGBA runtime not found')
        updates.append((package / 'TempGBA/EBOOT.PBP', runtime))
        for support in (package / 'TempGBA').glob('*.prx'):
            updates.append((support, shelf / 'TempGBA' / support.name))
        if args.version in ('1.07','1.08'):
            for source in (package / 'GBAdhoc').iterdir():
                if source.suffix.lower() in ('.pbp', '.prx'):
                    updates.append((source, shelf / 'GBAdhoc' / source.name))
        if args.version == '1.08':
            updates.extend([(package/'Atari/EBOOT.PBP',game/'psp2600/EBOOT.PBP'),
                            (package/'Snes9x/EBOOT.PBP',game/'s9xTYLme_mod/EBOOT.PBP')])
        # Old generated entries contain the old launcher. Back them up and
        # remove only verified GameShelf GBA entry folders, requiring regeneration.
        for entry in game.glob('GSH_GBA_*/EBOOT.PBP'):
            data = entry.read_bytes()
            pos = data.find(b'GSHXMB100TARGET!')
            if pos < 0 or pos + 276 > len(data) or struct.unpack_from('<I', data, pos+16)[0] != 3:
                raise RuntimeError('Unexpected generated entry; refusing removal: ' + str(entry))
            names = {p.name for p in entry.parent.iterdir()}
            if any(name != 'EBOOT.PBP' and name != '.DS_Store' and not name.startswith('._') for name in names):
                raise RuntimeError('Unexpected data inside generated entry: ' + str(entry.parent))
            entries.append(entry.parent)
    if args.backup_root.resolve().is_relative_to(shelf.resolve()):
        raise RuntimeError('Backup directory must be outside GAMESHELF')
    backup = args.backup_root / ('before_' + args.version + '_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    shutil.copytree(shelf, backup / 'GAMESHELF')
    for folder in entries:
        shutil.copytree(folder, backup / 'generated_gba_entries' / folder.name)
    for name in ('psp2600','s9xTYLme_mod') if args.version=='1.08' else ():
        shutil.copytree(game/name,backup/'shared_emulators'/name)
    def saved_path(dest):
        if dest.is_relative_to(shelf): return backup/'GAMESHELF'/dest.relative_to(shelf)
        return backup/'shared_emulators'/dest.relative_to(game)
    print('Backup:', backup, flush=True)
    prepared = []
    try:
        # Prepare and verify BOTH executable writes before replacing either.
        for source, dest in updates:
            temp = dest.with_name(dest.name + '.GSH_TMP')
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, temp)
            if digest(temp) != digest(source):
                raise RuntimeError('SD write verification failed: ' + str(dest))
            prepared.append((source, dest, temp))
        for source, dest, temp in prepared:
            temp.replace(dest)
            if digest(dest) != digest(source):
                raise RuntimeError('Installed EBOOT verification failed: ' + str(dest))
        for folder in entries:
            shutil.rmtree(folder)
        if args.version in ('1.04', '1.05', '1.06', '1.07', '1.08'):
            (shelf / 'TempGBA/SAVE_ID.TXT').unlink(missing_ok=True)
        if args.version=='1.08':
            (game/'s9xTYLme_mod/roms').mkdir(exist_ok=True)
        version_file.write_text(args.version + '\n')
    except Exception:
        for _, dest in updates:
            saved = saved_path(dest)
            if saved.exists(): shutil.copyfile(saved, dest)
            else: dest.unlink(missing_ok=True)
        for folder in entries:
            shutil.copytree(backup / 'generated_gba_entries' / folder.name, folder, dirs_exist_ok=True)
        for relative in ('INSTALL_VERSION.TXT', 'TempGBA/SAVE_ID.TXT'):
            saved, dest = backup / 'GAMESHELF' / relative, shelf / relative
            if saved.exists(): shutil.copyfile(saved, dest)
            else: dest.unlink(missing_ok=True)
        raise
    finally:
        for _, dest in updates:
            dest.with_name(dest.name + ".GSH_TMP").unlink(missing_ok=True)
    print('Installed and verified GameShelf', args.version)
    if entries:
        print('Old GBA entries backed up and removed:', len(entries))
    print('On PSP, open GameShelf and press X to regenerate the Memory Stick entries.')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError) as error:
        raise SystemExit('ERROR: ' + str(error))
