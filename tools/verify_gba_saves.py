#!/usr/bin/env python3
"""Compile and exercise the save-identity code used by launcher and emulator."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
C = r'''
#include <assert.h>
#include "gba_identity.h"
static unsigned char header[256];
static void select_game(const char *name) {
    char record[256];int n=gsh_identity_encode(record,sizeof(record),name,4096,header);
    FILE *f;assert(n>0);f=fopen("SAVE_ID.TXT","wb");assert(f);
    assert(fwrite(record,1,n,f)==(size_t)n);fclose(f);
}
static void path(char *out,const char *ext) {
    char name[256];char *dot;
    assert(gsh_identity_load("SAVE_ID.TXT",4096,header,name,sizeof(name))==0);
    dot=strrchr(name,'.');assert(dot);*dot=0;sprintf(out,"%s%s",name,ext);
}
static void save_value(const char *ext,int value) {
    char name[256];FILE *f;path(name,ext);f=fopen(name,"wb");assert(f);fputc(value,f);fclose(f);
}
static void expect_value(const char *ext,int value) {
    char name[256];FILE *f;path(name,ext);f=fopen(name,"rb");assert(f);assert(fgetc(f)==value);fclose(f);
}
int main(void) {
    char name[256],record[256];int n;FILE *f;
    memset(header,42,sizeof(header));
    /* A -> B -> A: independent SRAM saves, state slot and per-game settings. */
    select_game("Adventure A.gba");save_value(".sav",11);save_value("_0.svs",12);save_value(".cfg",13);
    select_game("Adventure B.GBA");save_value(".sav",21);save_value("_0.svs",22);save_value(".cfg",23);
    select_game("Adventure A.gba");expect_value(".sav",11);expect_value("_0.svs",12);expect_value(".cfg",13);
    select_game("Adventure B.GBA");expect_value(".sav",21);expect_value("_0.svs",22);expect_value(".cfg",23);
    assert(gsh_identity_load("missing.txt",4096,header,name,sizeof(name))<0);
    assert(gsh_identity_load("SAVE_ID.TXT",8192,header,name,sizeof(name))<0);
    header[100]^=1;assert(gsh_identity_load("SAVE_ID.TXT",4096,header,name,sizeof(name))<0);header[100]^=1;
    assert(gsh_identity_encode(record,sizeof(record),"../Other.gba",4096,header)<0);
    assert(gsh_identity_encode(record,sizeof(record),"Other\nName.gba",4096,header)<0);
    memset(name,'a',181);strcpy(name+181,".gba");assert(!gsh_valid_rom_name(name));
    n=gsh_identity_encode(record,sizeof(record),"Valid.gba",4096,header);assert(n>0);
    assert(gsh_identity_decode(record,n-1,4096,header,name,sizeof(name))<0);
    record[0]='X';assert(gsh_identity_decode(record,n,4096,header,name,sizeof(name))<0);
    f=fopen("SAVE_ID.TXT","wb");assert(f);for(n=0;n<300;n++)fputc('x',f);fclose(f);
    assert(gsh_identity_load("SAVE_ID.TXT",4096,header,name,sizeof(name))<0);
    /* An ambiguous old shared save is never adopted or overwritten. */
    f=fopen("game.sav","wb");assert(f);fputs("legacy",f);fclose(f);
    select_game("Adventure A.gba");save_value(".sav",31);
    f=fopen("game.sav","rb");assert(f);assert(fgetc(f)=='l');fclose(f);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    temp=Path(d);(temp/'test.c').write_text(C)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(ROOT/'shared'),str(temp/'test.c'),'-o',str(temp/'test')],check=True)
    subprocess.run([str(temp/'test')],cwd=temp,check=True)
print('PASS: A/B/A save, state and config isolation; identity/ROM checks; invalid inputs; legacy save preserved')
