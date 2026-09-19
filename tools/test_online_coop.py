import argparse
import json
from pathlib import Path
import subprocess
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import online_coop


class OnlineCoopTests(unittest.TestCase):
    def ns(self, **overrides):
        values = dict(
            action="host", exe=Path("game.exe"), transport="relay", players=2,
            port=51765, slot=None, connect=None, relay="relay.test",
            code="ABC123", input_delay=None, dry_run=True, game_arg=[],
        )
        values.update(overrides)
        return argparse.Namespace(**values)

    def test_relay_host_environment(self):
        env = online_coop.build_environment(self.ns())
        self.assertEqual(env["SM64DS_COMMS_ROLE"], "parent")
        self.assertEqual(env["SM64DS_COMMS_RELAY"], "relay.test:41234")
        self.assertEqual(env["SM64DS_COMMS_CODE"], "ABC123")
        self.assertEqual(env["SM64DS_VS_PLAYERS"], "2")

    def test_direct_join_environment(self):
        env = online_coop.build_environment(self.ns(
            action="join", transport="direct", connect="192.0.2.4",
            relay=None, code=None, players=4, slot=3, input_delay=4,
        ))
        self.assertEqual(env["SM64DS_COMMS_HOST"], "192.0.2.4:51765")
        self.assertEqual(env["SM64DS_COMMS_SLOT"], "3")
        self.assertEqual(env["SM64DS_COMMS_INPUT_DELAY"], "4")
        self.assertNotIn("SM64DS_COMMS_BIND_ANY", env)

    def test_direct_host_binds_publicly(self):
        env = online_coop.build_environment(self.ns(
            transport="direct", relay=None, code=None,
        ))
        self.assertEqual(env["SM64DS_COMMS_BIND_ANY"], "1")
        self.assertNotIn("SM64DS_COMMS_HOST", env)

    def test_code_is_normalized_and_bounded(self):
        self.assertEqual(online_coop.session_code("mario64"), "MARIO64")
        for bad in ("", "123456789", "two words", "é"):
            with self.assertRaises(argparse.ArgumentTypeError):
                online_coop.session_code(bad)

    def test_dry_run_does_not_require_an_executable(self):
        script = Path(online_coop.__file__).resolve()
        out = subprocess.check_output([
            sys.executable, str(script), "join", "--exe", "missing.exe",
            "--transport", "relay", "--relay", "relay.test", "--code", "LUIGI",
            "--dry-run",
        ], text=True)
        data = json.loads(out)
        self.assertEqual(data["environment"]["SM64DS_COMMS_ROLE"], "child")
        self.assertEqual(data["environment"]["SM64DS_COMMS_CODE"], "LUIGI")


if __name__ == "__main__":
    unittest.main()
