"""Exercise the compiled pack loader, including malformed mods and resource limits."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

PROBE = pathlib.Path(sys.argv.pop(1)).resolve()


class ResourcePacks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        self.pack = self.root / "probe"
        self.pack.mkdir()
        for name in ("body.bmd", "head.bmd", "idle.bca", "walk.bca", "skin.png"):
            (self.pack / name).touch()

    def run_pack(self, script, expected, diagnostic="", fixture=False):
        (self.pack / "pack.lua").write_text(script, encoding="ascii")
        result = subprocess.run(
            [str(PROBE), str(self.root)] + ([] if fixture else ["--validate"]),
            capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        self.assertIn(diagnostic, result.stderr)

    def character(self, fields=""):
        return ('sm64ds.character{ id=4, name="Probe", base=2, '
                'body="body.bmd", head_cap="head.bmd", head_no_cap="head.bmd", '
                + fields + '}')

    def test_valid_pack(self):
        self.run_pack(self.character('hitbox={radius=61,hurt_height=117}, '
                                   'animations={idle="idle.bca",walk="walk.bca"}') +
                      '\nsm64ds.texture{target="0123456789abcdef",source="skin.png"}',
                      0, fixture=True)

    def test_integer_wrap(self):
        self.run_pack(self.character().replace('id=4,', 'id=4294967300,'),
                      1, "integer range")

    def test_nan_hitbox(self):
        self.run_pack(self.character('hitbox={radius=0/0}'), 1, "must be a number")

    def test_duplicate_texture(self):
        self.run_pack('for i=1,2 do sm64ds.texture{target="0123456789abcdef",'
                      'source="skin.png"} end', 1, "duplicate texture")

    def test_memory_limit(self):
        self.run_pack('local s=string.rep("x",32*1024*1024)', 1, "memory")

    def test_instruction_limit(self):
        self.run_pack('while true do end', 1, "instruction budget")

    def test_path_escape(self):
        (self.root / "outside.bmd").touch()
        self.run_pack(self.character().replace('body="body.bmd"',
                                               'body="../outside.bmd"'),
                      1, "escapes its pack")

    def test_retail_slot(self):
        self.run_pack(self.character().replace('id=4,', 'id=0,'), 1, "4..255")

    def test_empty_root(self):
        self.run_pack('', 0)


if __name__ == "__main__":
    unittest.main()
