#!/usr/bin/env python3
"""Exercise the GBAdhoc adapter and GB/GBC entries using actual C sources."""
import ast
from pathlib import Path
import struct
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'third_party/GBAdhoc/gpsp/psp/main_psp.c').read_text()
start=source.index('   if (gameshelf_pending)\n')
block=source[start:source.index('   else if (have_variant)',start)]
head=r'''
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "rom_paths.h"
static struct { int console; } g_pcfg;
static int file_exists(const char *p){return access(p,F_OK)==0;}
static int console_from_rom_path(const char *p){return !strcasecmp(strrchr(p,'.'),".gbc")?5:4;}
static void evt_shutdown(void){} static void audio_stop(void){}
static void vid_term(void){} static void sceKernelExitGame(void){}
int main(int argc,char **argv){
 char rom_path[512],a[512],b[512];int gameshelf_pending=1;
 if(argc!=3)return 1;
'''
tail=r'''
 if(gameshelf_pending)return 2;
 if(psp_rom_path_suffix(a,sizeof(a),rom_path,".gb.sav") ||
    psp_rom_path_suffix(b,sizeof(b),rom_path,".gbc.sav") || !strcmp(a,b))return 3;
 printf("%d:%s",g_pcfg.console,rom_path);return 0;
}
'''
tree=ast.parse((ROOT/'tools/verify_xmb.py').read_text())
harness=next(n.value.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='HARNESS' for t in n.targets))
harness=harness.replace('return make_xmb_entry(argv[1],atoi(argv[2]))+1;', 'if(!strcmp(argv[1],"--cleanup")){printf("%d",gsh_cleanup_entries());return 0;}return make_xmb_entry(argv[1],atoi(argv[2]))+1;')
with tempfile.TemporaryDirectory() as d:
 root=Path(d);(root/'adapter.c').write_text(head+block+tail)
 subprocess.run(['cc','-I',str(ROOT/'third_party/GBAdhoc/gpsp/psp'),str(root/'adapter.c'),'-o',str(root/'adapter')],check=True)
 romdir='ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/'
 (root/romdir).mkdir(parents=True)
 for name,mode in [('Space Adventure.gb',4),('Space Adventure.gbc',5)]:
  (root/romdir/name).write_bytes(b'fixture, not a commercial ROM')
  result=subprocess.run([str(root/'adapter'),'--gameshelf',romdir+name],cwd=root,text=True,capture_output=True,check=True)
  assert result.stdout==f'{mode}:{romdir}{name}'
 for name in ['Missing.gb','../Outside.gb','Bad.gba','Nested/Game.gb',':bad.gb']:
  result=subprocess.run([str(root/'adapter'),'--gameshelf',romdir+name],cwd=root,text=True,capture_output=True,check=True)
  assert not result.stdout
 for path in ['x','ms0:/other.gb','']:
  assert not subprocess.run([str(root/'adapter'),'--gameshelf',path],cwd=root,text=True,capture_output=True,check=True).stdout
 (root/'generator.c').write_text(harness)
 subprocess.run(['cc','-I',str(ROOT/'src'),str(root/'generator.c'),'-o',str(root/'generator')],check=True)
 shelf=root/'ms0:/PSP/GAME/GAMESHELF';shelf.mkdir(parents=True)
 (shelf/'EBOOT.PBP').write_bytes((ROOT/'EBOOT.PBP').read_bytes())
 for name,mode,tag in [('Space Adventure.gb',4,'GB'),('Space Adventure.gbc',5,'GBC')]:
  assert subprocess.run([str(root/'generator'),name,str(mode)],cwd=root).returncode==1
  entry=next(shelf.parent.glob(f'GSH_{tag}_*/EBOOT.PBP'))
  data=entry.read_bytes();pos=data.index(b'GSHXMB100TARGET!')
  assert struct.unpack_from('<I',data,pos+16)[0]==mode
  assert data[pos+20:pos+276].split(b'\0')[0].decode()==name
  assert f'[{tag}] Space Adventure'.encode() in data
 result=subprocess.run([str(root/'generator'),'--cleanup'],cwd=root,capture_output=True,text=True,check=True)
 assert result.stdout=='0'
 (root/romdir/'Space Adventure.gb').unlink()
 result=subprocess.run([str(root/'generator'),'--cleanup'],cwd=root,capture_output=True,text=True,check=True)
 assert result.stdout=='1'
 assert len(list(shelf.parent.glob('GSH_GBC_*/EBOOT.PBP')))==1
print('PASS: GB/GBC direct selection, missing/invalid requests rejected, separate save suffixes, XMB identity/title and recoverable cleanup')
