#!/usr/bin/env python3
"""Create a curated first-install ZIP. Never read a memory card or user library."""
import argparse
import hashlib
from pathlib import Path
import zipfile
ROOT=Path(__file__).resolve().parents[1]

def package_setup(version):
 package=ROOT/('candidates/'+version if version=='1.07' else 'releases/v1/'+version.split('.')[1])
 # Validate all published payloads before packaging.
 for line in (package/'SHA256SUMS.txt').read_text().splitlines():
  digest,name=line.split('  ',1)
  if hashlib.sha256((package/name).read_bytes()).hexdigest()!=digest:
   raise RuntimeError('Checksum mismatch: '+name)
 payload={}
 def add(source,dest): payload[dest]=source.read_bytes()
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
 add(package/'tempgba-gameshelf.patch' if version=='1.06' else ROOT/'patches/tempgba-gameshelf.patch','SOURCE/tempgba-gameshelf.patch')
 add(ROOT/'patches/nesterj-autoload.patch','SOURCE/nesterj-autoload.patch')
 dirs=['PSP/GAME/TempGBA4PSP-Mod/roms/','PSP/GAME/Nester_AoEX_R3/roms/',
       'PSP/GAME/Nester_AoEX_R3/SAVE/','PSP/GAME/Nester_AoEX_R3/STATE/']
 if version=='1.07':
  for name in ['EBOOT.PBP','gbadhoc_me.prx','gbadhoc_home.prx','COPYING.txt','SOURCE.txt']:
   add(package/'GBAdhoc'/name,base+'GBAdhoc/'+name)
  add(ROOT/'patches/gbadhoc-gameshelf.patch','SOURCE/gbadhoc-gameshelf.patch')
  payload['TEST-BUILD.txt']=b'1.07 requires hardware confirmation. GB/GBC startup and saves need PSP testing.\n'
 payload[base+'INSTALL_VERSION.TXT']=(version+'\n').encode()
 refs=['Corresponding emulator source: https://github.com/fracarli/psp-gameshelf',
       'Clone with --recurse-submodules; use the recorded upstream commits.',
       'Apply the included patches inside the matching submodules.',
       'TempGBA also requires SOURCE/shared/gba_identity.h in the parent project.',
       'NesterJ: https://github.com/PSP-Archive/NesterJ-AoEX',
       'TempGBA: https://github.com/GrabowskiDev/TempGBA4PSP-Single-game']
 payload['SOURCE/README.txt']=('\n'.join(refs)+'\n').encode()
 payload['SHA256SUMS.txt']=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(payload.items())).encode()
 target=package/f'GameShelf-{version}-setup.zip'
 with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as archive:
  for directory in dirs: archive.writestr(directory,b'')
  for name,data in sorted(payload.items()):
   if Path(name).suffix.lower() in {'.gba','.gb','.gbc','.nes','.iso','.cso','.sav','.svs','.gsh','.bin'}:
    raise RuntimeError('Forbidden publication path: '+name)
   archive.writestr(name,data)
 with zipfile.ZipFile(target) as archive:
  if archive.testzip(): raise RuntimeError('ZIP validation failed')
 target.with_suffix(".zip.sha256").write_text(hashlib.sha256(target.read_bytes()).hexdigest()+"  "+target.name+"\n")
 print(target.relative_to(ROOT))
 return target

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--version',choices=['1.06','1.07'],required=True)
 package_setup(parser.parse_args().version)
