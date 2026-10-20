#!/usr/bin/env python3
"""Exercise actual extra-system paths, argv packets, XMB entries and cleanup."""
import ast
from pathlib import Path
import struct
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/main.c').read_text()
a=source.index('    if (view_mode >= 6 && view_mode <= 8)')
branch=source[a:source.index('    if (view_mode == 9)',a)]
head=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "extra_launch.h"
static int view_mode,cursor,atari_count=1,megadrive_count=1,snes_count=1,n64_count=1;
static char atari_roms[32][256],megadrive_roms[32][256],snes_roms[32][256],n64_roms[32][256];
static char path[256],argument[512];
static unsigned int argument_size;static int has_argument;
static const char *gsh_text(const char *s){return s;}
static void set_status(const char *s){(void)s;}
static int diagnostic_exists(const char *s,int d){(void)s;(void)d;return getenv("GSH_TEST_MISSING")==NULL;}
static int sceIoChdir(const char *s){(void)s;return 0;}
static void launch(void){
'''
tail=r'''
launch_application: ;
}
int main(int argc,char **argv){
 char full[512];const char *runtime;
 assert(argc==3);view_mode=atoi(argv[2]);
 strcpy(atari_roms[0],argv[1]);strcpy(megadrive_roms[0],argv[1]);strcpy(snes_roms[0],argv[1]);strcpy(n64_roms[0],argv[1]);
 launch();
 if(!has_argument)return 2;
 runtime=view_mode==6?"ms0:/PSP/GAME/psp2600/EBOOT.PBP":view_mode==7?
 "ms0:/PSP/GAME/PicoDrive/EBOOT.PBP":view_mode==8?"ms0:/PSP/GAME/s9xTYLme_mod/EBOOT.PBP":"ms0:/PSP/GAME/DaedalusX64/EBOOT.PBP";
 assert(!strcmp(argument,runtime));assert(!gsh_extra_path(full,sizeof(full),view_mode,argv[1]));
 assert(!strcmp(argument+strlen(argument)+1,full));
 assert(argument_size==strlen(runtime)+strlen(full)+2);
 return 0;
}
'''
tree=ast.parse((ROOT/'tools/verify_xmb.py').read_text())
harness=next(n.value.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='HARNESS' for t in n.targets))
harness=harness.replace('return make_xmb_entry(argv[1],atoi(argv[2]))+1;', 'if(!strcmp(argv[1],"--cleanup")){printf("%d",gsh_cleanup_entries());return 0;}return make_xmb_entry(argv[1],atoi(argv[2]))+1;')
with tempfile.TemporaryDirectory() as d:
 root=Path(d);(root/'packet.c').write_text(head+branch+tail)
 subprocess.run(['cc','-I',str(ROOT/'shared'),str(root/'packet.c'),'-o',str(root/'packet')],check=True)
 for mode,extensions in [(6,['a26','bin','rom']),(7,['md','bin','gen','smd']),(8,['smc','sfc']),(10,['z64','v64','n64'])]:
  for ext in extensions:
   subprocess.run([str(root/'packet'),'Space Adventure.'+ext,str(mode)],check=True)
  for name in ['../Outside.'+extensions[0],'Bad:Name.'+extensions[0],'.Hidden.'+extensions[0],'Bad.sav','Nested/Game.'+extensions[0],'A'*250+'.'+extensions[0]]:
   assert subprocess.run([str(root/'packet'),name,str(mode)]).returncode==2
 import os
 assert subprocess.run([str(root/'packet'),'Racing.z64','10'],env=dict(os.environ,GSH_TEST_MISSING='1')).returncode==2
 (root/'generator.c').write_text(harness)
 subprocess.run(['cc','-I',str(ROOT/'src'),str(root/'generator.c'),'-o',str(root/'generator')],check=True)
 game=root/'ms0:/PSP/GAME';shelf=game/'GAMESHELF';shelf.mkdir(parents=True)
 (shelf/'EBOOT.PBP').write_bytes((ROOT/'EBOOT.PBP').read_bytes())
 fixtures=[('Arcade.a26',6,'ATARI','psp2600','atari'),('Speed.md',7,'MD','PicoDrive','megadrive'),('Adventure.sfc',8,'SNES','s9xTYLme_mod','snes'),('Racing.z64',10,'N64','DaedalusX64','n64')]
 entries=[]
 for name,mode,tag,folder,art in fixtures:
  romdir=game/folder/('Roms' if mode==10 else 'roms');romdir.mkdir(parents=True);(romdir/name).write_bytes(b'owned test fixture')
  covers=shelf/'boxart'/art;covers.mkdir(parents=True)
  (covers/(Path(name).stem+'.xmb.gsh')).write_bytes(struct.pack('<H',0x07e0)*80*80)
  assert subprocess.run([str(root/'generator'),name,str(mode)],cwd=root).returncode==1
  entry=next(game.glob(f'GSH_{tag}_*/EBOOT.PBP'));entries.append(entry)
  data=entry.read_bytes();pos=data.index(b'GSHXMB100TARGET!');offset=struct.unpack_from('<8I',data,8)
  assert struct.unpack_from('<I',data,pos+16)[0]==mode
  assert data[pos+20:pos+276].split(b'\0')[0].decode()==name
  assert f'[{tag}] {Path(name).stem}'.encode() in data
  assert data[offset[1]:offset[1]+8]==b'\x89PNG\r\n\x1a\n'
  assert len(list(game.glob(f'GSH_{tag}_*')))==1
 # Display correction must preserve the original ROM launch identity.
 assert subprocess.run([str(root/'generator'),'Moonptrl.bin','6'],cwd=root).returncode==1
 moon=game/'GSH_ATARI_0D77F85F/EBOOT.PBP';data=moon.read_bytes();pos=data.index(b'GSHXMB100TARGET!')
 assert b'[ATARI] Moon Patrol (1983) (Atari)' in data
 assert data[pos+20:pos+276].split(b'\0')[0]==b'Moonptrl.bin'
 moon.unlink();moon.parent.rmdir()
 def cleanup():
  return int(subprocess.check_output([str(root/'generator'),'--cleanup'],cwd=root,text=True))
 assert cleanup()==0
 for name,mode,tag,folder,art in fixtures: (game/folder/('Roms' if mode==10 else 'roms')/name).unlink()
 (entries[0].parent/'progress.sta').write_bytes(b'protect extra user data')
 assert cleanup()==3 and entries[0].exists()
 assert len(list((root/'ms0:/PSP/GAMESHELF/REMOVED_ENTRIES').glob('*/EBOOT.PBP')))==3
print('PASS: Atari/MD/SNES/N64 argv, formats, path limits, XMB titles/icons and recoverable cleanup with user-data protection')
