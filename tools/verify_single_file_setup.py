#!/usr/bin/env python3
"""Exercise the PSP installer's actual transaction code on a host filesystem."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
PBP=ROOT/'candidates/1.09/GameShelfSetup/EBOOT.PBP'
records=json.loads((ROOT/'candidates/1.09/PAYLOAD-MANIFEST.json').read_text())
HARNESS=r'''
#include <stdio.h>
static int fault_rename(const char *,const char *);
#define rename fault_rename
#include "install.h"
#undef rename
static int fail_at,calls;
static int fault_rename(const char *a,const char *b) {
    if(fail_at && ++calls==fail_at) {errno=ENOSPC;return -1;}
    return rename(a,b);
}
int main(int argc,char **argv) {
    fail_at=argc>3?atoi(argv[3]):0;
    if(install_payload(argv[1],argv[2],NULL)) {fprintf(stderr,"%s\n",install_error);return 2;}
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    tmp=Path(d);(tmp/'test.c').write_text(HARNESS)
    exe=tmp/'test'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-I',str(ROOT/'installer'),'-I',str(ROOT/'build/installer'),str(tmp/'test.c'),'-o',str(exe)],check=True)
    def run(card,pbp=PBP,fail_at=0):
        return subprocess.run([str(exe),str(pbp),str(card),str(fail_at)],capture_output=True)
    fresh=tmp/'fresh'
    assert run(fresh).returncode==0
    for r in records:
        assert hashlib.sha256((fresh/r['path']).read_bytes()).hexdigest()==r['sha256'],r['path']
    assert (fresh/'PSP/GAME/s9xTYLme_mod/roms').is_dir()
    assert (fresh/'PSP/GAME/TempGBA4PSP-Mod/roms').is_dir()
    assert run(fresh).returncode==0,'Repeat install must preserve matching files'
    for line in (fresh/'PSP/GAMESHELF/DISTRIBUTION/SHA256SUMS.txt').read_text().splitlines():
        digest,name=line.split('  ',1);assert hashlib.sha256((fresh/name).read_bytes()).hexdigest()==digest,name
    # A late conflict must stop the entire installation before any writes.
    conflict=tmp/'conflict';existing=conflict/records[-1]['path'];existing.parent.mkdir(parents=True);existing.write_bytes(b'original engine or custom setting')
    assert run(conflict).returncode==2 and existing.read_bytes()==b'original engine or custom setting'
    assert len([p for p in conflict.rglob('*') if p.is_file()])==1
    # Failure after successful commits must remove only files this attempt introduced.
    failure=tmp/'failure';save=failure/'PSP/GAME/GAMESHELF/TempGBA/save/Adventure.sav';save.parent.mkdir(parents=True);save.write_bytes(b'keep save')
    assert run(failure,fail_at=12).returncode==2
    assert [p for p in failure.rglob('*') if p.is_file()]==[save] and save.read_bytes()==b'keep save'
    # Corrupted and truncated payloads must not leave a partial installation.
    corrupt=tmp/'corrupt.PBP';data=bytearray(PBP.read_bytes());psar=struct.unpack_from('<I',data,36)[0]
    data[psar+records[8]['offset']]^=1;corrupt.write_bytes(data)
    for pbp,label in [(corrupt,'corrupt'),(tmp/'short.PBP','short')]:
        if label=='short':pbp.write_bytes(data[:-1])
        card=tmp/label
        assert run(card,pbp).returncode==2
        assert not [p for p in card.rglob('*') if p.is_file()]
print('PASS: actual installer extraction, 77 payload hashes, directory creation, idempotency, conflict preflight, rollback with saves preserved, corrupted/truncated payload rejection')
