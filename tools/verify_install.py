#!/usr/bin/env python3
"""Exercise paired 1.06 installation on a temporary card with legacy saves."""
from pathlib import Path
import subprocess
import struct
import tempfile
import shutil
import importlib.util
import sys
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
    root=Path(d);card=root/'card';shelf=card/'PSP/GAME/GAMESHELF'
    (shelf/'TempGBA/save').mkdir(parents=True)
    (shelf/'TempGBA/state').mkdir()
    (shelf/'EBOOT.PBP').write_bytes((ROOT/'releases/v1/03/EBOOT.PBP').read_bytes())
    old_runtime=(ROOT/'releases/v1/00/TempGBA/EBOOT.PBP').read_bytes()
    (shelf/'TempGBA/EBOOT.PBP').write_bytes(old_runtime)
    (shelf/'TempGBA/save/game.sav').write_bytes(b'legacy SRAM must survive')
    (shelf/'TempGBA/state/game_0.svs').write_bytes(b'legacy state must survive')
    entry=card/'PSP/GAME/GSH_GBA_TEST';entry.mkdir()
    b=bytearray((ROOT/'releases/v1/03/EBOOT.PBP').read_bytes())
    pos=b.index(b'GSHXMB100TARGET!');struct.pack_into('<I',b,pos+16,3)
    (entry/'EBOOT.PBP').write_bytes(b)
    game=card/'PSP/GAME'
    mod=game/'TempGBA4PSP-Mod';(mod/'roms').mkdir(parents=True)
    (mod/'gba_bios.bin').write_bytes(bytes(16384))
    for name in ('tempgba_me.prx','cp_fix_gbk.prx','exception.prx'):
        (mod/name).write_bytes(b'test support file')
    nester=game/'Nester_AoEX_R3';(nester/'roms').mkdir(parents=True)
    (nester/'EBOOT.PBP').write_bytes((ROOT/'releases/nesterj-autoload-v1/EBOOT.PBP').read_bytes())
    backups=root/'backups'
    cmd=['python3',str(ROOT/'tools/install_release.py'),'--sd-root',str(card),'--version','1.06','--backup-root',str(backups)]
    check=cmd+['--check']
    subprocess.run(check,check=True,capture_output=True)
    assert not backups.exists() and entry.exists()
    (mod/'gba_bios.bin').write_bytes(b'bad BIOS size')
    assert subprocess.run(check,capture_output=True).returncode!=0
    assert subprocess.run(cmd,capture_output=True).returncode!=0
    assert not backups.exists() and entry.exists()
    (mod/'gba_bios.bin').write_bytes(bytes(16384))
    nester_binary=(nester/'EBOOT.PBP').read_bytes()
    (nester/'EBOOT.PBP').write_bytes(b'\0PBP')
    assert subprocess.run(check,capture_output=True).returncode!=0
    assert not backups.exists()
    (nester/'EBOOT.PBP').write_bytes(nester_binary)
    subprocess.run(cmd,check=True,capture_output=True)
    assert not entry.exists()
    backup=next(backups.iterdir())
    assert (backup/'generated_gba_entries/GSH_GBA_TEST/EBOOT.PBP').read_bytes()==b
    assert (shelf/'EBOOT.PBP').read_bytes()==(ROOT/'releases/v1/06/EBOOT.PBP').read_bytes()
    assert (shelf/'TempGBA/EBOOT.PBP').read_bytes()==(ROOT/'releases/v1/06/TempGBA/EBOOT.PBP').read_bytes()
    assert (backup/'GAMESHELF/TempGBA/EBOOT.PBP').read_bytes()==old_runtime
    assert (shelf/'TempGBA/save/game.sav').read_bytes()==b'legacy SRAM must survive'
    assert (shelf/'TempGBA/state/game_0.svs').read_bytes()==b'legacy state must survive'
    # A second install must preserve existing named saves as well.
    (shelf/'TempGBA/save/Adventure.sav').write_bytes(b'per-game SRAM')
    subprocess.run(cmd,check=True,capture_output=True)
    assert (shelf/'TempGBA/save/Adventure.sav').read_bytes()==b'per-game SRAM'
    # A failure replacing the second executable restores both original files.
    previous_main=(ROOT/'releases/v1/03/EBOOT.PBP').read_bytes()
    (shelf/'EBOOT.PBP').write_bytes(previous_main)
    (shelf/'TempGBA/EBOOT.PBP').write_bytes(old_runtime)
    spec=importlib.util.spec_from_file_location('installer',ROOT/'tools/install_release.py')
    installer=importlib.util.module_from_spec(spec);spec.loader.exec_module(installer)
    original_replace=Path.replace
    failed=[False]
    def fail_runtime_once(self,target):
        if self.name=='EBOOT.PBP.GSH_TMP' and self.parent.name=='TempGBA' and not failed[0]:
            failed[0]=True;raise OSError('simulated second executable replacement failure')
        return original_replace(self,target)
    original_argv=sys.argv
    try:
        sys.argv=cmd[1:];Path.replace=fail_runtime_once
        try: installer.main()
        except OSError: pass
        else: raise AssertionError('Expected simulated write failure')
    finally:
        Path.replace=original_replace;sys.argv=original_argv
    assert (shelf/'EBOOT.PBP').read_bytes()==previous_main
    assert (shelf/'TempGBA/EBOOT.PBP').read_bytes()==old_runtime
    assert (shelf/'TempGBA/save/Adventure.sav').read_bytes()==b'per-game SRAM'
    # Reject a partial downgrade; the two runtimes must be restored together.
    downgrade=cmd.copy();downgrade[downgrade.index('1.06')]='1.03'
    assert subprocess.run(downgrade,capture_output=True).returncode!=0
print('PASS: read-only preflight, bad BIOS rejected before writes, paired installation, entry backup/regeneration, old and named saves preserved, rollback on runtime write failure, partial downgrade rejected')
