"""Release validation rejects incomplete, corrupt, or mislabeled packages."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location(
    'release', Path(__file__).resolve().parents[1] / '.github/scripts/release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.version = '0.1.0'
        self.commit = 'a' * 40
        self.name = f'XP-Control-{self.version}-{self.commit[:8]}'
        self.metadata = {'version': self.version, 'commit': self.commit, 'dirty': False,
                         'dll_sha256': hashlib.sha256(b'dll').hexdigest()}
        with zipfile.ZipFile(self.directory / (self.name + '.zip'), 'w') as archive:
            archive.writestr('SKSE/Plugins/XPControl.dll', b'dll')
            archive.writestr('SKSE/Plugins/XPControl.ini', '[Curve]\n')
        with zipfile.ZipFile(self.directory / (self.name + '-symbols.zip'), 'w') as archive:
            archive.writestr('XPControl.pdb', b'symbols')
        self.write_metadata()

    def write_metadata(self):
        (self.directory / 'build-info.json').write_text(json.dumps(self.metadata))
        self.write_checksums()

    def write_checksums(self):
        files = sorted(path for path in self.directory.iterdir() if path.name != 'SHA256SUMS.txt')
        (self.directory / 'SHA256SUMS.txt').write_text(''.join(
            f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n' for path in files))

    def verify(self):
        return release.verify_package(self.directory, self.version, self.commit)

    def test_release_and_prerelease_tags(self):
        for tag in ['v0.1.0', 'v0.1.0-rc.1']:
            release.check_tag(tag, self.version)
        for tag in ['v0.2.0', '0.1.0', 'v0.1.0/', 'v0.1.0-', 'v0.1.0\n']:
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                release.check_tag(tag, self.version)

    def test_complete_package(self):
        self.assertEqual(len(self.verify()), 4)

    def test_dirty_wrong_version_or_wrong_commit(self):
        for key, value in [('dirty', True), ('version', '0.2.0'), ('commit', 'b' * 40)]:
            with self.subTest(key=key):
                old = self.metadata[key]
                self.metadata[key] = value
                self.write_metadata()
                with self.assertRaises(ValueError):
                    self.verify()
                self.metadata[key] = old

    def test_corrupt_download(self):
        with (self.directory / (self.name + '.zip')).open('ab') as output:
            output.write(b'corrupted')
        with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
            self.verify()

    def test_wrong_dll(self):
        self.metadata['dll_sha256'] = '0' * 64
        self.write_metadata()
        with self.assertRaisesRegex(ValueError, 'DLL checksum'):
            self.verify()

    def test_missing_or_extra_assets(self):
        extra = self.directory / 'extra.zip'
        extra.write_bytes(b'extra')
        with self.assertRaises(ValueError):
            self.verify()
        extra.unlink()
        (self.directory / (self.name + '-symbols.zip')).unlink()
        with self.assertRaises(ValueError):
            self.verify()

    def test_duplicate_and_path_checksum_entries(self):
        manifest = self.directory / 'SHA256SUMS.txt'
        contents = manifest.read_text()
        for line in [contents.splitlines()[0], '0' * 64 + '  ../outside.zip']:
            with self.subTest(line=line):
                manifest.write_text(contents + line + '\n')
                with self.assertRaises(ValueError):
                    self.verify()

    def test_wrong_symbol_filename(self):
        with zipfile.ZipFile(self.directory / (self.name + '-symbols.zip'), 'w') as archive:
            archive.writestr('Other.pdb', b'symbols')
        self.write_checksums()
        with self.assertRaisesRegex(ValueError, 'debug-symbol'):
            self.verify()


if __name__ == '__main__':
    unittest.main()
