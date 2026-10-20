#!/usr/bin/env python3
"""Check first-install archives and a GB/GBC update with a rollback fault."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from check_installation import check_installation
with tempfile.TemporaryDirectory() as d:
 root=Path(d)
 for version,folder in [('1.06','releases/v1/06'),('1.07','releases/v1/07')]:
  package=ROOT/folder
  with zipfile.ZipFile(package/f'GameShelf-{version}-setup.zip') as archive:
   names=archive.namelist()
   assert all(not n.startswith(('/', '../')) and '/../' not in n for n in names)
   assert not any(Path(n).suffix.lower() in {'.gba','.gb','.gbc','.nes','.bin','.iso','.cso','.sav','.svs','.gsh'} for n in names)
   for line in archive.read('SHA256SUMS.txt').decode().splitlines():
    digest,name=line.split('  ',1);assert hashlib.sha256(archive.read(name)).hexdigest()==digest
   for name in names:
    if name.endswith('EBOOT.PBP'):
     data=archive.read(name);off=struct.unpack_from('<8I',data,8)
     assert data[:4]==b'\0PBP' and sorted(off)==list(off) and off[6]<off[7]==len(data)
   card=root/version;archive.extractall(card)
  issues=check_installation(card,package)
  assert len(issues)==1 and issues[0].endswith('TempGBA4PSP-Mod/gba_bios.bin'),issues
  (card/'PSP/GAME/TempGBA4PSP-Mod/gba_bios.bin').write_bytes(bytes(16384))
  assert not check_installation(card,package)
  assert not (card/'PSP/PSP').exists()
 card=root/'1.06';shelf=card/'PSP/GAME/GAMESHELF'
 original=card/'PSP/GAME/GBAdhoc410';original.mkdir();(original/'EBOOT.PBP').write_bytes(b'existing original runtime')
 (original/'progress.gb.sav').write_bytes(b'existing original save')
 (shelf/'TempGBA/save').mkdir();save=shelf/'TempGBA/save/Adventure.sav';save.write_bytes(b'keep')
 cmd=[sys.executable,str(ROOT/'tools/install_release.py'),'--sd-root',str(card),'--version','1.07','--backup-root',str(root/'backups')]
 # Inject a failure at the newly added GB executable, after launcher/core writes.
 spec=importlib.util.spec_from_file_location('installer',ROOT/'tools/install_release.py')
 installer=importlib.util.module_from_spec(spec);spec.loader.exec_module(installer)
 main=(shelf/'EBOOT.PBP').read_bytes();core=(shelf/'TempGBA/EBOOT.PBP').read_bytes()
 previous=Path.replace;argv=sys.argv;failed=[False]
 def fail_once(self,target):
  if self.parent.name=='GBAdhoc' and self.name=='EBOOT.PBP.GSH_TMP' and not failed[0]:
   failed[0]=True;raise OSError('simulated GB runtime write failure')
  return previous(self,target)
 try:
  Path.replace=fail_once;sys.argv=cmd[1:]
  try: installer.main()
  except OSError: pass
  else: raise AssertionError('fault not reached')
 finally: Path.replace=previous;sys.argv=argv
 assert failed[0] and (shelf/'EBOOT.PBP').read_bytes()==main and (shelf/'TempGBA/EBOOT.PBP').read_bytes()==core
 assert not list((shelf/'GBAdhoc').glob('*'))
 subprocess.run(cmd,capture_output=True,check=True)
 package=ROOT/'releases/v1/07'
 for name in ('EBOOT.PBP','gbadhoc_me.prx','gbadhoc_home.prx'):
  assert (shelf/'GBAdhoc'/name).read_bytes()==(package/'GBAdhoc'/name).read_bytes()
 assert (original/'EBOOT.PBP').read_bytes()==b'existing original runtime'
 assert (original/'progress.gb.sav').read_bytes()==b'existing original save' and save.read_bytes()==b'keep'
print('PASS: curated first-install ZIPs, payload hashes/PBP headers, fresh-card checks, GB runtime update/rollback and original saves preserved')
