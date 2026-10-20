#!/usr/bin/env python3
"""Verify first-install 1.08 and rollback across both shared emulator folders."""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from check_installation import check_installation
package=ROOT/'candidates/1.08'
with tempfile.TemporaryDirectory() as d:
 root=Path(d);card=root/'card'
 with zipfile.ZipFile(package/'GameShelf-1.08-setup.zip') as z:
  assert not z.testzip()
  for line in z.read('SHA256SUMS.txt').decode().splitlines():
   digest,name=line.split('  ',1);assert hashlib.sha256(z.read(name)).hexdigest()==digest
  assert all(Path(n).suffix.lower() not in {'.gba','.gb','.gbc','.nes','.a26','.bin','.rom','.gen','.smd','.smc','.sfc','.spc','.sav','.srm','.sta','.gsh'} for n in z.namelist())
  assert not any(n.endswith(('music.zip','logo.zip')) for n in z.namelist())
  z.extractall(card)
 game=card/'PSP/GAME';shelf=game/'GAMESHELF';(game/'TempGBA4PSP-Mod/gba_bios.bin').write_bytes(bytes(16384))
 assert not check_installation(card,package)
 # Replace binaries with older valid PBP fixtures to exercise an actual update.
 old=(ROOT/'releases/v1/07/EBOOT.PBP').read_bytes()
 targets=[shelf/'EBOOT.PBP',game/'psp2600/EBOOT.PBP',game/'s9xTYLme_mod/EBOOT.PBP']
 for p in targets:p.write_bytes(old)
 save=game/'psp2600/save/sav_Arcade_0.sta';save.write_bytes(b'keep Atari state')
 snes=game/'s9xTYLme_mod/SAVES/Adventure.SRM';snes.write_bytes(b'keep SNES battery')
 module=game/'s9xTYLme_mod/mediaengine.prx';prior_module=module.read_bytes()
 pico=(game/'PicoDrive/EBOOT.PBP').read_bytes()
 spec=importlib.util.spec_from_file_location('installer',ROOT/'tools/install_release.py')
 installer=importlib.util.module_from_spec(spec);spec.loader.exec_module(installer)
 cmd=[sys.executable,str(ROOT/'tools/install_release.py'),'--sd-root',str(card),'--version','1.08','--backup-root',str(root/'backups')]
 prior=Path.replace;argv=sys.argv;failed=[False]
 def fault(self,target):
  if self.parent.name=='s9xTYLme_mod' and self.name=='EBOOT.PBP.GSH_TMP' and not failed[0]:
   failed[0]=True;raise OSError('simulated external SNES replacement failure')
  return prior(self,target)
 try:
  sys.argv=cmd[1:];Path.replace=fault
  try: installer.main()
  except OSError: pass
  else: raise AssertionError('fault not reached')
 finally: Path.replace=prior;sys.argv=argv
 assert failed[0] and all(p.read_bytes()==old for p in targets)
 assert save.read_bytes()==b'keep Atari state' and snes.read_bytes()==b'keep SNES battery'
 assert module.read_bytes()==prior_module
 subprocess.run(cmd,capture_output=True,check=True)
 for target,payload in [(targets[0],'EBOOT.PBP'),(targets[1],'Atari/EBOOT.PBP'),(targets[2],'Snes9x/EBOOT.PBP')]:
  assert target.read_bytes()==(package/payload).read_bytes()
 assert save.read_bytes()==b'keep Atari state' and snes.read_bytes()==b'keep SNES battery'
 assert module.read_bytes()==prior_module and (game/'PicoDrive/EBOOT.PBP').read_bytes()==pico
 # An unsupported PicoDrive must be rejected before any further backup/write.
 (game/'PicoDrive/EBOOT.PBP').write_bytes(old)
 assert any('PicoDrive 2.05' in issue for issue in check_installation(card,package))
print('PASS: complete 1.08 setup, payload hashes, cross-folder rollback, shared save/config preservation and supported PicoDrive preflight')
