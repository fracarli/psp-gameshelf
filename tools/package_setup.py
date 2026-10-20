#!/usr/bin/env python3
"""Create a curated first-install ZIP without reading a card or user library."""
import argparse
import hashlib
from pathlib import Path
import zipfile
ROOT=Path(__file__).resolve().parents[1]


def package_setup(version):
    package=ROOT/'candidates'/version
    if not package.is_dir(): package=ROOT/'releases/v1'/version.split('.')[1]
    for line in (package/'SHA256SUMS.txt').read_text().splitlines():
        digest,name=line.split('  ',1)
        if hashlib.sha256((package/name).read_bytes()).hexdigest()!=digest:
            raise RuntimeError('Checksum mismatch: '+name)
    payload={}
    def add(source,dest): payload[dest]=source.read_bytes()
    def add_tree(source,dest):
        for file in sorted(source.rglob('*')):
            if file.is_file(): add(file,dest+file.relative_to(source).as_posix())
    base='PSP/GAME/GAMESHELF/'
    add(package/'EBOOT.PBP',base+'EBOOT.PBP')
    for name in ['EBOOT.PBP','ku_bridge.prx','tempgba_me.prx','cp_fix_gbk.prx','exception.prx','COPYING.txt','SOURCE.txt']:
        add(package/'TempGBA'/name,base+'TempGBA/'+name)
    for name in ['tempgba_me.prx','cp_fix_gbk.prx','exception.prx']:
        add(package/'TempGBA'/name,'PSP/GAME/TempGBA4PSP-Mod/'+name)
    add(ROOT/'releases/v1/00/NesterJ-EBOOT.PBP','PSP/GAME/Nester_AoEX_R3/EBOOT.PBP')
    add(ROOT/'third_party/NesterJ-AoEX/nesterj/docs/COPYING.txt','LICENSES/NesterJ-COPYING.txt')
    add(ROOT/'LICENSE','LICENSES/GameShelf-MIT.txt')
    add(ROOT/'docs/START-HERE.txt','START-HERE.txt')
    add(ROOT/'shared/gba_identity.h','SOURCE/shared/gba_identity.h')
    for name in ['tempgba-gameshelf.patch','nesterj-autoload.patch']:
        source=package/name
        add(source if source.is_file() else ROOT/'patches'/name,'SOURCE/'+name)
    dirs=['PSP/GAME/TempGBA4PSP-Mod/roms/','PSP/GAME/Nester_AoEX_R3/roms/',
          'PSP/GAME/Nester_AoEX_R3/SAVE/','PSP/GAME/Nester_AoEX_R3/STATE/']
    if version in ('1.07','1.08'):
        for name in ['EBOOT.PBP','gbadhoc_me.prx','gbadhoc_home.prx','COPYING.txt','SOURCE.txt']:
            add(package/'GBAdhoc'/name,base+'GBAdhoc/'+name)
        patch=package/'gbadhoc-gameshelf.patch'
        add(patch if patch.exists() else ROOT/'patches/gbadhoc-gameshelf.patch','SOURCE/gbadhoc-gameshelf.patch')
    if version=='1.08':
        add_tree(package/'Atari','PSP/GAME/psp2600/')
        add_tree(package/'Snes9x','PSP/GAME/s9xTYLme_mod/')
        add_tree(package/'PicoDrive','PSP/GAME/PicoDrive/')
        for folder in ['psp2600','s9xTYLme_mod','PicoDrive']:
            dirs.append('PSP/GAME/'+folder+'/roms/')
        for folder in ['save','state','set','kbd','cht','scr','txt']:
            dirs.append('PSP/GAME/psp2600/'+folder+'/')
        dirs.append('PSP/GAME/s9xTYLme_mod/SAVES/')
        add(ROOT/'shared/extra_launch.h','SOURCE/shared/extra_launch.h')
        for patch in ['psp2600-gameshelf.patch','snes9x-gameshelf.patch']:
            add(ROOT/'patches'/patch,'SOURCE/'+patch)
        if package.parent.name=='candidates':
            payload['TEST-BUILD.txt']=b'1.08 awaits hardware confirmation.\n'
        else:
            payload['HARDWARE-STATUS.txt']=b'The user confirmed the installed 1.08 setup works on PSP-3004, 6.61 PRO-C.\nA fresh-card installation and dedicated per-system save/state checks remain separate validation steps.\n'
    payload[base+'INSTALL_VERSION.TXT']=(version+'\n').encode()
    payload['SOURCE/README.txt']=b'''Corresponding source: https://github.com/fracarli/psp-gameshelf
Clone with --recurse-submodules and apply the included patches inside matching
submodules at the recorded upstream commits. Shared headers belong in the
parent project. Build commands: patches/README.md. Each runtime retains its
upstream terms and has source/license references alongside its executable.
PicoDrive is the unmodified upstream PSP 2.05 release with its original license.
'''
    forbidden={'.gba','.gb','.gbc','.nes','.iso','.cso','.sav','.svs','.srm','.gsh','.bin','.a26','.rom','.mdrom','.gen','.smd','.smc','.sfc','.spc'}
    payload['SHA256SUMS.txt']=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(payload.items())).encode()
    target=package/f'GameShelf-{version}-setup.zip'
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as archive:
        for directory in dirs: archive.writestr(directory,b'')
        for name,data in sorted(payload.items()):
            if Path(name).suffix.lower() in forbidden: raise RuntimeError('Forbidden payload: '+name)
            archive.writestr(name,data)
    with zipfile.ZipFile(target) as archive:
        if archive.testzip(): raise RuntimeError('ZIP validation failed')
    target.with_suffix('.zip.sha256').write_text(hashlib.sha256(target.read_bytes()).hexdigest()+'  '+target.name+'\n')
    print(target.relative_to(ROOT))
    return target


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version',choices=['1.06','1.07','1.08'],required=True)
    package_setup(parser.parse_args().version)
