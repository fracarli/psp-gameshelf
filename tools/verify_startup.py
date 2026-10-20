#!/usr/bin/env python3
"""Run actual launcher preparation functions against a temporary memory card."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/main.c').read_text()
functions=source[source.index('static int copy_file('):source.index('static int is_ps1_folder(',source.index('static int copy_file('))]
HEAD=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "gba_identity.h"
typedef int SceUID;
typedef struct { long st_size; } SceIoStat;
#define ENTRY_LENGTH 256
#define PSP_O_RDONLY O_RDONLY
#define PSP_O_WRONLY O_WRONLY
#define PSP_O_CREAT O_CREAT
#define PSP_O_TRUNC O_TRUNC
#define sceIoOpen open
#define sceIoClose close
#define sceIoRead read
#define sceIoWrite write
#define sceIoMkdir mkdir
#define sceIoRemove unlink
static unsigned int gsh_copied_bytes,gsh_rom_copies,last_percent;
static int gsh_launch_active=1;
static int sceIoGetstat(const char *p,SceIoStat *s){struct stat t;int r=stat(p,&t);if(!r)s->st_size=t.st_size;return r;}
static int has_extension(const char *p,const char *e){const char *dot=strrchr(p,'.');return dot&&!strcasecmp(dot,e);}
static void gsh_launch_stage(const char *s){(void)s;}
static void gsh_copy_progress(unsigned int n,unsigned int total){last_percent=total?(unsigned long long)n*100/total:0;}
'''
TAIL=r'''
int main(void){
    char name[256];unsigned char header[256];FILE *f;unsigned int first;
    assert(prepare_gba_runtime("Adventure.gba")==0);
    assert(gsh_rom_copies==1&&last_percent==100);
    f=fopen("ms0:/PSP/GAME/GAMESHELF/TempGBA/roms/game.gba","rb");assert(f);
    assert(fread(header,1,256,f)==256);fclose(f);
    assert(gsh_identity_load(GSH_IDENTITY_PATH,262144,header,name,sizeof(name))==0);
    assert(!strcmp(name,"Adventure.gba"));
    first=gsh_copied_bytes;gsh_copied_bytes=0;gsh_rom_copies=0;
    assert(prepare_gba_runtime("Adventure.gba")==0);
    assert(gsh_rom_copies==1&&gsh_copied_bytes==262144&&first>gsh_copied_bytes);
    /* Self-copy used by legacy adapters must never truncate the ROM. */
    assert(copy_file("ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/Adventure.gba","ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/Adventure.gba")==0);
    f=fopen("ms0:/PSP/GAME/TempGBA4PSP-Mod/roms/Adventure.gba","rb");assert(f);
    assert(fseek(f,0,SEEK_END)==0&&ftell(f)==262144);fclose(f);
    f=fopen("ms0:/PSP/GAME/GAMESHELF/TempGBA/save/Adventure.sav","rb");assert(f&&fgetc(f)==77);fclose(f);
    /* The unused legacy transport file is preserved, not recopied. */
    f=fopen("ms0:/PSP/GAME/GAMESHELF/roms/game.gba","rb");assert(f&&fgetc(f)=='L');fclose(f);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    root=Path(d);(root/'test.c').write_text(HEAD+functions+TAIL)
    base=root/'ms0:/PSP/GAME'
    original=base/'TempGBA4PSP-Mod';(original/'roms').mkdir(parents=True)
    (original/'roms/Adventure.gba').write_bytes(bytes(range(256))*1024)
    for name in ('gba_bios.bin','tempgba_me.prx','cp_fix_gbk.prx','exception.prx'):
        (original/name).write_bytes(b'support data')
    shelf=base/'GAMESHELF';(shelf/'TempGBA/save').mkdir(parents=True)
    (shelf/'TempGBA/save/Adventure.sav').write_bytes(bytes([77]))
    (shelf/'roms').mkdir();(shelf/'roms/game.gba').write_bytes(b'Legacy transport')
    subprocess.run(['cc','-std=c99','-Wall','-I',str(ROOT/'shared'),str(root/'test.c'),'-o',str(root/'test')],check=True)
    subprocess.run([str(root/'test')],cwd=root,check=True)
print('PASS: one ROM copy, unchanged support files not rewritten, progress reaches 100%, identity and saves retained, self-copy safe')
