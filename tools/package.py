#!/usr/bin/env python3
"""Build a manager-installable archive from tested build outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--output', type=Path, default=Path('dist'))
args = parser.parse_args()
version = re.search(r'project\(XPControl VERSION ([\d.]+)', (root / 'CMakeLists.txt').read_text())[1]
dll = args.build / 'XPControl.dll'
pdb = args.build / 'XPControl.pdb'
if not dll.is_file() or not pdb.is_file():
    parser.error('Both the built DLL and its PDB are required.')
subprocess.run([sys.executable, str(root / 'tools/audit-dll.py'), str(dll)], check=True)
commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
dirty = bool(subprocess.check_output(['git', '-C', str(root), 'status', '--porcelain'], text=True).strip())
args.output.mkdir(parents=True, exist_ok=True)
name = f'XP-Control-{version}-{commit[:8]}' + ('-dirty' if dirty else '')
with zipfile.ZipFile(args.output / (name + '.zip'), 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.write(dll, 'SKSE/Plugins/XPControl.dll')
    archive.write(root / 'package/SKSE/Plugins/XPControl.ini', 'SKSE/Plugins/XPControl.ini')
    for file in ['README.md', 'THIRD_PARTY_NOTICES.md', 'LICENSE', 'EXCEPTIONS.md']:
        archive.write(root / file, file)
    licenses = {file.name: file for file in (root / 'licenses').iterdir() if file.is_file()}
    share = args.build / 'vcpkg_installed/x64-windows-static-md/share'
    for dependency in ['directxmath', 'directxtk', 'fmt', 'rapidcsv', 'spdlog']:
        copyright = share / dependency / 'copyright'
        if not copyright.is_file():
            raise RuntimeError(f'Missing built dependency license: {copyright}')
        licenses[dependency + '.txt'] = copyright
    commonlib = args.build / '_deps/commonlibsse-src'
    openvr = commonlib / 'extern/openvr/LICENSE'
    if not openvr.is_file():
        raise RuntimeError('Missing OpenVR submodule license')
    licenses['OpenVR.txt'] = openvr
    for name, file in sorted(licenses.items()):
        archive.write(file, 'licenses/' + name)
with zipfile.ZipFile(args.output / (name + '-symbols.zip'), 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.write(pdb, pdb.name)
metadata = {'version': version, 'commit': commit, 'dirty': dirty, 'runtimes': ['SE', 'AE (Steam/GOG, including 1.7.99)', 'VR'], 'commonlib': '13.1.0', 'commonlib_commit': '12d61f3c39f7d4ba1b1ab543df2bca0e613dc747',
            'dll_sha256': hashlib.sha256(dll.read_bytes()).hexdigest()}
(args.output / 'build-info.json').write_text(json.dumps(metadata, indent=2) + '\n')
files = sorted(args.output.glob('*.zip')) + [args.output / 'build-info.json']
(args.output / 'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in files))
print(json.dumps(metadata, indent=2))
