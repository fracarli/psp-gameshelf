#!/usr/bin/env python3
"""Check English fallback, PSP Italian selection and translated format strings."""
from pathlib import Path
import json
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
header=(ROOT/'src/language.h').read_text()
pairs=re.findall(r'\{\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*\}',header)
format_pattern=r'%(?:[-+ #0]*)(?:\d+)?(?:\.\d+)?[diuoxXfFeEgGcs%]'
for english,italian in pairs:
    assert re.findall(format_pattern,json.loads(english)) == re.findall(format_pattern,json.loads(italian))
C=r'''
#include <assert.h>
#include "language.h"
int main(void) {
    int i;
    assert(strcmp(gsh_text("Ready."),"Ready.")==0);
    gsh_set_language(5);
    assert(strcmp(gsh_text("Ready."),"Pronto.")==0);
    assert(strcmp(gsh_text("ERROR"),"ERRORE")==0);
    assert(strcmp(gsh_text("Untranslated text"),"Untranslated text")==0);
    for(i=-1;i<=12;i++) {
        if(i==5)continue;
        gsh_set_language(i);
        assert(strcmp(gsh_text("Ready."),"Ready.")==0);
        assert(strcmp(gsh_text("ERROR"),"ERROR")==0);
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d);(p/'test.c').write_text(C)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: English default/fallback, PSP Italian selection, unknown strings and translation format compatibility')
