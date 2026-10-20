#!/usr/bin/env python3
"""Build the adapted GB/GBC runtime with GBAdhoc's upstream release profile."""
import os
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
core=ROOT/'third_party/GBAdhoc/gpsp'
flags=re.search(r'CORE_COMMON="(.*?)"',(core/'tools/build.sh').read_text(),re.S)[1].split()
for cwd,args in [(core,['platform=psp1','clean']),
                 (core,['platform=psp1','-j2','GPSP_PROFILE=release',*flags]),
                 (core/'psp',['clean']),
                 (core/'psp',['-j2','EXTRA_DEFS=-DGPSP_PLAYABLE -DVID_TRIPLE -DGPSP_PROFILE_RELEASE=1','ME_CATCH=1','PSP_EBOOT_TITLE=GameShelf GB/GBC'])]:
 subprocess.run(['make',*args],cwd=cwd,check=True)
print('Built GB/GBC runtime and support modules. No memory-card files changed.')
