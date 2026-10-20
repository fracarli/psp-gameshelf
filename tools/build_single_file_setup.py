#!/usr/bin/env python3
"""Build a self-contained PSP first installer from curated, verified payloads."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import struct
import subprocess
import zipfile
import zlib
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/installer'
CANDIDATE = ROOT / 'candidates/1.09'


def build():
    BUILD.mkdir(parents=True, exist_ok=True)
    CANDIDATE.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env['PATH'] = str(Path.home()/'pspdev/bin') + os.pathsep + env['PATH']
    launcher = CANDIDATE/'EBOOT.PBP'
    if not launcher.is_file():
        subprocess.run(['make', '-B'], cwd=ROOT, env=env, check=True)
        launcher = ROOT/'EBOOT.PBP'
    else:
        expected = next(line.split('  ',1)[0] for line in (CANDIDATE/'SHA256SUMS.txt').read_text().splitlines() if line.endswith('  EBOOT.PBP'))
        assert hashlib.sha256(launcher.read_bytes()).hexdigest() == expected
    stable = ROOT / 'releases/v1/08'
    archive = stable / 'GameShelf-1.08-setup.zip'
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == (archive.with_suffix('.zip.sha256').read_text().split()[0])
    files, dirs = {}, []
    with zipfile.ZipFile(archive) as z:
        for line in z.read('SHA256SUMS.txt').decode().splitlines():
            digest, name = line.split('  ', 1)
            assert hashlib.sha256(z.read(name)).hexdigest() == digest, name
        for info in z.infolist():
            name = info.filename
            if info.is_dir():
                dirs.append(name)
                continue
            data = z.read(name)
            if name == 'PSP/GAME/GAMESHELF/EBOOT.PBP':
                data = launcher.read_bytes()
                assert data.startswith(b'\0PBP') and data.count(b'GSHXMB100TARGET!') == 1
                assert b'PSP GameShelf 1.09' in data
            elif name == 'PSP/GAME/GAMESHELF/INSTALL_VERSION.TXT':
                data = b'1.09\n'
            if not name.startswith('PSP/'):
                name = 'PSP/GAMESHELF/DISTRIBUTION/' + name
            assert '..' not in name and ':' not in name and '\\' not in name
            assert Path(name).suffix.lower() not in {'.gba', '.gb', '.gbc', '.nes', '.bin', '.iso', '.cso', '.sav', '.srm', '.a26', '.rom', '.gen', '.smd', '.smc', '.sfc', '.spc'}
            assert name not in files and not name.startswith('PSP/GAME/GameShelfSetup/')
            files[name] = data
    files['PSP/GAMESHELF/DISTRIBUTION/HARDWARE-STATUS.txt'] = b'GameShelf 1.09 single-file setup is a candidate. First-install extraction and the 1.09 launcher await PSP validation. The bundled emulator engines are unchanged from stable 1.08.\n'
    files['PSP/GAMESHELF/DISTRIBUTION/START-HERE.txt'] = (ROOT/'docs/START-HERE.txt').read_bytes()
    files['PSP/GAMESHELF/DISTRIBUTION/SHA256SUMS.txt'] = ''.join(
        hashlib.sha256(data).hexdigest() + '  ' + name + '\n'
        for name, data in sorted(files.items())
        if name != 'PSP/GAMESHELF/DISTRIBUTION/SHA256SUMS.txt'
    ).encode()
    blob, records = bytearray(), []
    for name, data in sorted(files.items()):
        records.append({'path': name, 'offset': len(blob), 'size': len(data), 'crc': zlib.crc32(data), 'sha256': hashlib.sha256(data).hexdigest()})
        blob.extend(data)
    (BUILD/'payload.dat').write_bytes(blob)
    header = '#include <stdint.h>\ntypedef struct { const char *path; uint32_t offset,size,crc; } GshPayloadFile;\n'
    header += f'#define GSH_PAYLOAD_SIZE {len(blob)}u\n#define GSH_PAYLOAD_COUNT {len(records)}u\n'
    header += 'static const GshPayloadFile gsh_payload[] = {\n'
    header += ''.join('{%s,%du,%du,%du},\n' % (json.dumps(r['path']), r['offset'], r['size'], r['crc']) for r in records) + '};\n'
    header += f'#define GSH_DIRECTORY_COUNT {len(dirs)}u\nstatic const char *gsh_directories[] = {{\n'
    header += ''.join(json.dumps(name) + ',\n' for name in dirs) + '};\n'
    (BUILD/'payload.h').write_text(header)
    (BUILD/'manifest.json').write_text(json.dumps(records, indent=2)+'\n')
    subprocess.run(['make', '-B'], cwd=ROOT/'installer', env=env, check=True)
    base = (ROOT/'installer/EBOOT.PBP').read_bytes()
    offsets = struct.unpack_from('<8I', base, 8)
    assert offsets[-1] == len(base)
    target = CANDIDATE/'GameShelfSetup/EBOOT.PBP'
    target.parent.mkdir(exist_ok=True)
    target.write_bytes(base+blob)
    if launcher.resolve() != (CANDIDATE/'EBOOT.PBP').resolve():
        shutil.copy2(launcher, CANDIDATE/'EBOOT.PBP')
    shutil.copy2(BUILD/'manifest.json', CANDIDATE/'PAYLOAD-MANIFEST.json')
    wrapper = CANDIDATE/'GameShelf-1.09-single-file-setup.zip'
    with zipfile.ZipFile(wrapper, 'w', zipfile.ZIP_DEFLATED) as z:
        z.write(target, 'PSP/GAME/GameShelfSetup/EBOOT.PBP')
    wrapper.with_suffix('.zip.sha256').write_text(hashlib.sha256(wrapper.read_bytes()).hexdigest()+'  '+wrapper.name+'\n')
    names = ['EBOOT.PBP', 'GameShelfSetup/EBOOT.PBP', 'PAYLOAD-MANIFEST.json', 'INSTALL.txt']
    (CANDIDATE/'SHA256SUMS.txt').write_text(''.join(
        hashlib.sha256((CANDIDATE/name).read_bytes()).hexdigest()+'  '+name+'\n'
        for name in names))
    print(f'Single installer: {target.relative_to(ROOT)}, {target.stat().st_size:,} bytes; {len(records)} files')
    return target


if __name__ == '__main__':
    build()
