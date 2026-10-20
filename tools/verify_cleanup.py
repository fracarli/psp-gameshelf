#!/usr/bin/env python3
"""Exercise actual PSP cleanup code with missing, live and untrusted entries."""
import ast
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
tree=ast.parse((ROOT/'tools/verify_xmb.py').read_text())
harness=next(node.value.value for node in tree.body if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='HARNESS' for t in node.targets))
harness=harness.replace('return make_xmb_entry(argv[1],atoi(argv[2]))+1;', 'if(!strcmp(argv[1],"--cleanup")){printf("%d\\n",gsh_cleanup_entries());return 0;}return make_xmb_entry(argv[1],atoi(argv[2]))+1;')
with tempfile.TemporaryDirectory() as d:
    root=Path(d);(root/'test.c').write_text(harness)
    subprocess.run(['cc','-I',str(ROOT/'src'),str(root/'test.c'),'-o',str(root/'test')],check=True)
    game=root/'ms0:/PSP/GAME';shelf=game/'GAMESHELF';shelf.mkdir(parents=True)
    (shelf/'EBOOT.PBP').write_bytes((ROOT/'EBOOT.PBP').read_bytes())
    gba=game/'TempGBA4PSP-Mod/roms';gba.mkdir(parents=True)
    nes=game/'Nester_AoEX_R3/roms';nes.mkdir(parents=True)
    def create(name,mode):
        before=set(game.glob('GSH_*'))
        assert subprocess.run([str(root/'test'),name,str(mode)],cwd=root).returncode in (1,2)
        created=set(game.glob('GSH_*'))-before
        return next(iter(created)) if created else None
    def cleanup():
        result=subprocess.run([str(root/'test'),'--cleanup'],cwd=root,capture_output=True,text=True,check=True)
        return int(result.stdout.strip())
    live=create('Live.gba',3);(gba/'Live.gba').write_bytes(b'ROM stays untouched')
    missing=create('Missing.gba',3)
    (missing/'.DS_Store').write_bytes(b'metadata')
    protected=create('Protected.gba',3);(protected/'progress.sav').write_bytes(b'never delete me')
    corrupt=game/'GSH_GBA_DEADBEEF';corrupt.mkdir();(corrupt/'EBOOT.PBP').write_bytes(b'not ours')
    mismatch=create('Mismatch.nes',9);mismatch.rename(game/'GSH_NES_BADC0FFE')
    # Missing library: leave every entry alone, even if its ROM is absent.
    gba.rename(gba.with_name('roms-offline'))
    assert cleanup()==0
    gba.with_name('roms-offline').rename(gba)
    save=shelf/'TempGBA/save/Missing.sav';save.parent.mkdir(parents=True);save.write_bytes(b'keep this save')
    archive=root/'ms0:/PSP/GAMESHELF/REMOVED_ENTRIES'
    assert cleanup()==1
    assert not missing.exists() and live.exists() and protected.exists() and corrupt.exists()
    assert (protected/'progress.sav').read_bytes()==b'never delete me'
    assert save.read_bytes()==b'keep this save'
    first=next(archive.glob('*/EBOOT.PBP')).read_bytes()
    assert next(archive.glob('*/.DS_Store')).read_bytes()==b'metadata'
    assert cleanup()==0
    create('Missing.gba',3);assert cleanup()==1
    assert len(list(archive.glob('*/EBOOT.PBP')))==2
    assert all(p.read_bytes()==first for p in archive.glob('*/EBOOT.PBP'))
    # Ownership and actual file existence, not the scanner's 32-entry list,
    # decide cleanup. Existing ROMs beyond that limit must be retained.
    for i in range(35):
        name=f'Present {i}.nes';create(name,9);(nes/name).write_bytes(b'ROM')
    assert cleanup()==0
print('PASS: recoverable cleanup; live ROMs, saves, extra data, corrupt/foreign entries and offline libraries protected')
