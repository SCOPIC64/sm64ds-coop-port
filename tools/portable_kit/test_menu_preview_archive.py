import hashlib
import base64
import io
import json
import unittest
import zipfile
from tools.portable_kit.publish_menu_preview import validate_archive


class ArchiveChecks(unittest.TestCase):
    def fixture(self, extra=None):
        files = {'sm64ds coop.exe': b'MZtest', 'README.txt': b'Read me',
                 'LICENSE': b'License', 'THIRD-PARTY-NOTICES.txt': b'Notices'}
        files.update(extra or {})
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, 'w') as archive:
            for name, data in files.items():
                archive.writestr(name, data)
        manifest = {'archive_files': list(files),
                    'exe_sha256': hashlib.sha256(files['sm64ds coop.exe']).hexdigest()}
        return buffer.getvalue(), manifest

    def test_previous_and_portable_folder_archives(self):
        validate_archive(*self.fixture())
        script = b'-- reviewed Lua mod'
        data, manifest = self.fixture({'mods/README.txt': b'Mods',
                                      'mods/resource-packs/README.txt': b'Textures',
                                      'texture-work/capture/README.txt': b'Capture',
                                      'mods/sm64-movement/main.lua': script})
        manifest['bundled_mod_sha256'] = hashlib.sha256(script).hexdigest()
        validate_archive(data, manifest, script)
        with self.assertRaises(AssertionError):
            validate_archive(data, manifest, b'changed script')

    def test_unexpected_files_and_paths_are_rejected(self):
        for name in ('rom.nds', 'build/assets/romdata.bin', '../outside.txt',
                     'mods/unknown/main.lua', 'extra.dll'):
            with self.subTest(name=name), self.assertRaises(AssertionError):
                validate_archive(*self.fixture({name: b'not allowed'}))

    def test_online_archive_contains_only_public_profile_fields(self):
        public = b'\x06\x02\x00\x00\x00\x24\x00\x00RSA1\x00\x08\x00\x00\x01\x00\x01\x00' + b'\x01' * 256
        profile = {'relay_addr': 'cairo-pot.tun.ply.gg:41702',
                   'discovery_key': base64.b64encode(public).decode()}
        def online(value):
            return self.fixture({'JOIN-STEPS.txt': b'Join by room code',
                                 'server-profile.json': json.dumps(value).encode()})
        validate_archive(*online(profile))
        with self.assertRaises(AssertionError):
            validate_archive(*online(dict(profile, agent_secret='must not ship')))
        with self.assertRaises(AssertionError):
            validate_archive(*online(dict(profile, relay_addr='127.0.0.1:41234')))
        with self.assertRaises(AssertionError):
            private = b'\x07' + public[1:]
            validate_archive(*online(dict(profile, discovery_key=base64.b64encode(private).decode())))

    def test_hash_mismatch_is_rejected(self):
        data, manifest = self.fixture()
        manifest['exe_sha256'] = '0' * 64
        with self.assertRaises(AssertionError):
            validate_archive(data, manifest)


if __name__ == '__main__':
    unittest.main()
