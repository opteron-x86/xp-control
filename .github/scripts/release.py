#!/usr/bin/env python3
"""Validate version tags and publish tested XP Control packages."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def check_tag(tag, version):
    match = re.fullmatch(r'v(\d+\.\d+\.\d+)(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?', tag)
    if not match or match[1] != version:
        raise ValueError(f'Release tag must be v{version} or v{version}-<prerelease>.')


def verify_package(directory, version, commit):
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('A full source commit is required.')
    name = f'XP-Control-{version}-{commit[:8]}'
    expected = {name + '.zip', name + '-symbols.zip', 'build-info.json'}
    checksums = {}
    for line in (directory / 'SHA256SUMS.txt').read_text().splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  ([^/\\]+)', line)
        if not match or match[2] in checksums:
            raise ValueError('Invalid or duplicate checksum entry.')
        checksums[match[2]] = match[1]
    if set(checksums) != expected or {p.name for p in directory.iterdir()} != expected | {'SHA256SUMS.txt'}:
        raise ValueError('The package must contain both archives, build metadata, and checksums.')
    for filename, digest in checksums.items():
        if hashlib.sha256((directory / filename).read_bytes()).hexdigest() != digest:
            raise ValueError(f'Checksum mismatch: {filename}')
    metadata = json.loads((directory / 'build-info.json').read_text())
    if metadata.get('version') != version or metadata.get('commit') != commit or metadata.get('dirty') is not False:
        raise ValueError('Package metadata must match the version and clean source commit.')
    with zipfile.ZipFile(directory / (name + '.zip')) as archive:
        if archive.testzip() or 'SKSE/Plugins/XPControl.ini' not in archive.namelist():
            raise ValueError('Invalid installable archive.')
        dll = archive.read('SKSE/Plugins/XPControl.dll')
        if hashlib.sha256(dll).hexdigest() != metadata.get('dll_sha256'):
            raise ValueError('DLL checksum mismatch.')
    with zipfile.ZipFile(directory / (name + '-symbols.zip')) as archive:
        if archive.testzip() or archive.namelist() != ['XPControl.pdb']:
            raise ValueError('Invalid debug-symbol archive.')
    return sorted(directory / filename for filename in expected | {'SHA256SUMS.txt'})


def gh(*args):
    return subprocess.check_output(['gh', *map(str, args)], text=True).strip()


def publish(tag, version, commit, files):
    result = subprocess.run(['gh', 'release', 'view', tag, '--json', 'isDraft,url'],
                            capture_output=True, text=True)
    release = json.loads(result.stdout) if result.returncode == 0 else None
    if release and not release['isDraft']:
        # A completed release can be verified again when a workflow is rerun.
        with tempfile.TemporaryDirectory() as temporary:
            gh('release', 'download', tag, '--dir', temporary)
            verify_package(Path(temporary), version, commit)
        return release['url']

    package_name = f'XP-Control-{version}-{commit[:8]}'
    repository = os.environ['GH_REPO']
    server = os.environ.get('GITHUB_SERVER_URL', 'https://github.com')
    notes = f'''XP Control configures character XP requirements through an INI file and an SKSE Menu Framework panel.

Install **`{package_name}.zip`** with a mod manager. It contains `SKSE/Plugins/XPControl.dll`, the default configuration, and license notices.

- `{package_name}-symbols.zip` contains `XPControl.pdb` for debugging.
- `SHA256SUMS.txt` provides checksums for the archives and build metadata.
- `build-info.json` records the source commit, dependency version, and DLL checksum.

Requires the matching SKSE and Address Library for Skyrim SE, AE, or VR. SKSE Menu Framework 3 provides the in-game panel.

Source: [{commit[:8]}]({server}/{repository}/commit/{commit}). [Build and tests]({server}/{repository}/actions/runs/{os.environ['GITHUB_RUN_ID']}).
'''
    with tempfile.TemporaryDirectory() as temporary:
        notes_file = Path(temporary) / 'release-notes.md'
        notes_file.write_text(notes)
        if release:
            gh('release', 'upload', tag, *files, '--clobber')
        else:
            gh('release', 'create', tag, *files, '--verify-tag', '--draft',
               '--title', f'XP Control {tag}', '--notes-file', notes_file)
        options = ['--prerelease', '--latest=false'] if '-' in tag else ['--prerelease=false']
        gh('release', 'edit', tag, '--draft=false', '--title', f'XP Control {tag}',
           '--notes-file', notes_file, *options)
    return gh('release', 'view', tag, '--json', 'url', '--jq', '.url')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--dist', type=Path)
    parser.add_argument('--commit')
    args = parser.parse_args()
    version = re.search(r'project\(XPControl VERSION ([\d.]+)', (ROOT / 'CMakeLists.txt').read_text())[1]
    check_tag(args.tag, version)
    print(f'{args.tag} matches project version {version}.', flush=True)
    if args.dist is None:
        return
    if not args.commit:
        parser.error('--commit is required when publishing.')
    files = verify_package(args.dist, version, args.commit)
    url = publish(args.tag, version, args.commit, files)
    print(url)
    if summary := os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(summary, 'a') as output:
            output.write(f'[Download XP Control {args.tag}]({url})\n')


if __name__ == '__main__':
    main()
