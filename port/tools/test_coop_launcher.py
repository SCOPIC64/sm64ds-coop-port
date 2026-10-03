"""Check portable session setup without starting the game or opening sockets."""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

KIT = pathlib.Path(sys.argv.pop(1)).resolve()


class CoopLauncher(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        shutil.copyfile(KIT / "coop.ps1", self.root / "coop.ps1")
        (self.root / "play.bat").write_text(
            "@echo off\nset SM64DS_\nexit /b 7\n", newline="\r\n")

    def launch(self, *args):
        env = dict(os.environ, SM64DS_WINDOW_SELFTEST="1", SM64DS_COMMS_HOST="stale")
        return subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy",
            "RemoteSigned", "-File", str(self.root / "coop.ps1"), *args],
            env=env, capture_output=True, text=True, timeout=20)

    def test_host(self):
        result = self.launch("-Mode", "host")
        self.assertEqual(result.returncode, 7, result.stderr)
        self.assertIn("SM64DS_COMMS_ROLE=parent", result.stdout)
        self.assertIn("SM64DS_COMMS_BIND_ANY=1", result.stdout)
        self.assertIn("SM64DS_PARTY=1", result.stdout)
        self.assertNotIn("SM64DS_WINDOW_SELFTEST=", result.stdout)
        self.assertNotIn("SM64DS_COMMS_HOST=", result.stdout)

    def test_join(self):
        result = self.launch("-Mode", "join", "-Address", "127.0.0.1", "-Port", "44700")
        self.assertEqual(result.returncode, 7, result.stderr)
        self.assertIn("SM64DS_COMMS_ROLE=child", result.stdout)
        self.assertIn("SM64DS_COMMS_HOST=127.0.0.1:44700", result.stdout)
        self.assertIn("coop-join.sav", result.stdout)

    def test_invalid_address(self):
        result = self.launch("-Mode", "join", "-Address", "not-an-ip")
        self.assertEqual(result.returncode, 1)
        self.assertIn("valid IPv4", result.stderr)

    def test_invalid_port(self):
        result = self.launch("-Mode", "host", "-Port", "65535")
        self.assertEqual(result.returncode, 1)
        self.assertNotIn("SM64DS_COMMS_ROLE=parent", result.stdout)


if __name__ == "__main__":
    unittest.main()
