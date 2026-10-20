#!/usr/bin/env python3
"""Exercise the actual C PBP generator with host filesystem shims."""
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
typedef int SceUID;
typedef struct { long st_size; } SceIoStat;
#define PSP_O_RDONLY O_RDONLY
#define PSP_O_WRONLY O_WRONLY
#define PSP_O_CREAT O_CREAT
#define PSP_O_TRUNC O_TRUNC
#define sceIoOpen open
#define sceIoClose close
#define sceIoRead read
#define sceIoWrite write
#define sceIoMkdir mkdir
#define sceIoRename rename
#define sceIoRemove unlink
#define pspDebugScreenPrintf printf
static int sceIoGetstat(const char *p,SceIoStat *s) { struct stat t; int r=stat(p,&t); if(!r)s->st_size=t.st_size;return r; }
static volatile struct { unsigned char magic[16]; unsigned int mode; char rom[256]; } xmb_target={ "GSHXMB100TARGET!",0,"" };
static int gba_count,nes_count;
static char gba_roms[32][256],nes_roms[32][256];
static void scan_gba_roms(void) {}
static void scan_nes_roms(void) {}
static int diagnostic_exists(const char *p,int d) { (void)d;return access(p,F_OK)==0; }
#include "xmb.h"
int main(int argc,char **argv) { return make_xmb_entry(argv[1],atoi(argv[2]))+1; }
'''

def u32(b, offset):
    return struct.unpack_from('<I', b, offset)[0]

with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    (temp/'test.c').write_text(HARNESS)
    subprocess.run(['cc', '-I', str(ROOT/'src'), str(temp/'test.c'), '-o', str(temp/'test')], check=True)
    base = temp/'ms0:/PSP/GAME/GAMESHELF'
    base.mkdir(parents=True)
    original = (ROOT/'EBOOT.PBP').read_bytes()
    (base/'EBOOT.PBP').write_bytes(original)
    # Real 1.00 entries must be upgradeable in place by 1.01.
    old_template = (ROOT/'releases/v1/00/EBOOT.PBP').read_bytes()
    (base/'EBOOT.PBP').write_bytes(old_template)
    assert subprocess.run([str(temp/'test'), 'Example Adventure.gba', '3'], cwd=temp).returncode == 1
    (base/'EBOOT.PBP').write_bytes(original)
    covers = base/'boxart/gba'
    covers.mkdir(parents=True)
    # Red RGB565 cover. NES intentionally has no cover (fallback icon).
    (covers/'Example Adventure.gsh').write_bytes(struct.pack('<H', 0xf800)*128*160)
    for name, mode in [('Example Adventure.gba', 3), ('Example.nes', 9)]:
        assert subprocess.run([str(temp/'test'), name, str(mode)], cwd=temp).returncode == (2 if mode == 3 else 1)
        assert subprocess.run([str(temp/'test'), name, str(mode)], cwd=temp).returncode == 2
    assert subprocess.run([str(temp/'test'), '../bad.gba', '3'], cwd=temp).returncode == 0
    generated = list((base.parent).glob('GSH_*/EBOOT.PBP'))
    assert len(generated) == 2
    for file in generated:
        b = file.read_bytes()
        marker = b.index(b'GSHXMB100TARGET!')
        mode = u32(b, marker+16)
        assert mode in (3, 9)
        assert b[marker+20:marker+276].split(b'\0')[0].endswith(b'.gba' if mode == 3 else b'.nes')
        offsets = struct.unpack_from('<8I', b, 8)
        assert list(offsets) == sorted(offsets) and offsets[-1] == len(b)
        icon = b[offsets[1]:offsets[2]]
        if mode == 3:
            assert icon.startswith(b'\x89PNG\r\n\x1a\n')
            pos, image = 8, b''
            while pos < len(icon):
                size = struct.unpack_from('>I', icon, pos)[0]
                kind, payload = icon[pos+4:pos+8], icon[pos+8:pos+8+size]
                assert struct.unpack_from('>I', icon, pos+8+size)[0] == zlib.crc32(kind+payload)
                if kind == b'IHDR': assert struct.unpack_from('>II',payload) == (144,80)
                if kind == b'IDAT': image += payload
                pos += size+12
            raw = zlib.decompress(image)
            assert len(raw) == (144*3+1)*80
            assert raw[1+40*3:1+41*3] == bytes([255,0,0])
            assert raw[1:4] == bytes([18,24,38])
        else:
            assert icon == original[u32(original,12):u32(original,16)]
        sfo = offsets[0]
        for i in range(u32(b, sfo+16)):
            e = sfo+20+i*16
            key = struct.unpack_from('<H', b, e)[0]+sfo+u32(b,sfo+8)
            if b[key:key+6] == b'TITLE\0':
                assert u32(b,e+8) == 128
                v = sfo+u32(b,sfo+12)+u32(b,e+12)
                assert b[v:v+5] in (b'[GBA]',b'[NES]')
        # Only the embedded target changes inside the executable.
        old_exec = bytearray(original[u32(original,32):u32(original,36)])
        new_exec = bytearray(b[offsets[6]:offsets[7]])
        pos = old_exec.index(b'GSHXMB100TARGET!')
        new_exec[pos+16:pos+276] = old_exec[pos+16:pos+276]
        assert old_exec == new_exec
    # Refuse overwriting an unrelated EBOOT with the same generated folder name.
    victim = generated[0]
    victim.write_bytes(b'unrelated application')
    other_name = 'Example Adventure.gba' if 'GBA' in victim.parent.name else 'Example.nes'
    other_mode = '3' if 'GBA' in victim.parent.name else '9'
    assert subprocess.run([str(temp/'test'), other_name, other_mode], cwd=temp).returncode == 0
    assert victim.read_bytes() == b'unrelated application'
    (base/'EBOOT.PBP').write_bytes(b'bad template')
    assert subprocess.run([str(temp/'test'), 'Other.nes', '9'], cwd=temp).returncode == 0
print('PASS: actual C generator, NES/GBA metadata, executable preservation, 1.00 upgrades, PNG CRC/pixels, missing-cover fallback, collisions, invalid paths and corrupt template')
