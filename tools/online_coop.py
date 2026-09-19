#!/usr/bin/env python3
"""Launch the SM64DS PC port with a validated online co-op configuration."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess

DEFAULT_GAME_PORT = 51765
DEFAULT_RELAY_PORT = 41234


def endpoint(value: str, default_port: int) -> str:
    value = value.strip()
    if not value:
        raise argparse.ArgumentTypeError("endpoint cannot be empty")
    if value.startswith("["):
        if "]" not in value:
            raise argparse.ArgumentTypeError("invalid IPv6 endpoint")
        return value if value.rsplit("]", 1)[1].startswith(":") else f"{value}:{default_port}"
    if value.count(":") == 0:
        return f"{value}:{default_port}"
    host, port = value.rsplit(":", 1)
    if not host or not port.isdigit() or not 1 <= int(port) <= 65535:
        raise argparse.ArgumentTypeError("endpoint must be HOST or HOST:PORT")
    return value


def session_code(value: str) -> str:
    if not value or len(value) > 8 or not value.isascii() or not value.isalnum():
        raise argparse.ArgumentTypeError(
            "session code must be 1-8 ASCII letters or digits"
        )
    return value.upper()


def build_environment(args: argparse.Namespace) -> dict[str, str]:
    env = {
        "SM64DS_COMMS_ROLE": "parent" if args.action == "host" else "child",
        "SM64DS_COMMS_FANOUT": "1",
        "SM64DS_COMMS_REPORT": "1",
        "SM64DS_VS_PLAYERS": str(args.players),
    }
    if args.port != DEFAULT_GAME_PORT:
        env["SM64DS_COMMS_PORT"] = str(args.port)
    if args.slot is not None:
        env["SM64DS_COMMS_SLOT"] = str(args.slot)
    if args.input_delay is not None:
        env["SM64DS_COMMS_INPUT_DELAY"] = str(args.input_delay)

    if args.transport == "direct":
        if args.action == "host":
            env["SM64DS_COMMS_BIND_ANY"] = "1"
        else:
            env["SM64DS_COMMS_HOST"] = endpoint(args.connect, args.port)
    elif args.transport == "relay":
        env["SM64DS_COMMS_RELAY"] = endpoint(args.relay, DEFAULT_RELAY_PORT)
        env["SM64DS_COMMS_CODE"] = args.code
    return env


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Start a validated SM64DS online co-op host or joiner."
    )
    p.add_argument("action", choices=("host", "join"))
    p.add_argument("--exe", type=Path, required=True, help="PC-port executable")
    p.add_argument(
        "--transport", choices=("local", "direct", "relay"), default="relay"
    )
    p.add_argument("--players", type=int, default=2, choices=range(2, 17))
    p.add_argument("--port", type=int, default=DEFAULT_GAME_PORT)
    p.add_argument("--slot", type=int, choices=range(1, 16))
    p.add_argument("--connect", help="direct-mode parent address")
    p.add_argument("--relay", help="relay address (default port 41234)")
    p.add_argument("--code", type=session_code, help="shared relay session code")
    p.add_argument("--input-delay", type=int, choices=range(0, 9))
    p.add_argument("--dry-run", action="store_true")
    p.add_argument(
        "--game-arg", action="append", default=[],
        help="argument forwarded to the game; repeat for multiple arguments",
    )
    return p


def validate(p: argparse.ArgumentParser, args: argparse.Namespace) -> None:
    if not 1025 <= args.port <= 65519:
        p.error("--port must be between 1025 and 65519")
    if args.action == "host" and args.slot is not None:
        p.error("--slot is only valid when joining")
    if args.transport == "direct" and args.action == "join" and not args.connect:
        p.error("direct join requires --connect HOST[:PORT]")
    if args.transport == "direct" and args.relay:
        p.error("--relay is not valid in direct mode")
    if args.transport == "relay":
        if not args.relay or not args.code:
            p.error("relay mode requires --relay HOST[:PORT] and --code CODE")
        if args.connect:
            p.error("--connect is not valid in relay mode")
    elif args.code:
        p.error("--code is only valid in relay mode")
    if args.transport == "local" and (args.connect or args.relay):
        p.error("local mode does not take --connect or --relay")
    if not args.dry_run and not args.exe.is_file():
        p.error(f"executable not found: {args.exe}")


def main(argv: list[str] | None = None) -> int:
    p = parser()
    args = p.parse_args(argv)
    validate(p, args)
    settings = build_environment(args)
    command = [str(args.exe.resolve()), *args.game_arg]
    if args.dry_run:
        print(json.dumps({"command": command, "environment": settings}, indent=2))
        return 0

    child_env = os.environ.copy()
    child_env.update(settings)
    mode = f"{args.action}/{args.transport}"
    print(f"Starting SM64DS co-op ({mode}, {args.players} players)")
    if args.transport == "relay":
        print(f"Relay {settings['SM64DS_COMMS_RELAY']}, code {args.code}")
    proc = subprocess.Popen(command, cwd=args.exe.resolve().parent, env=child_env)
    try:
        return proc.wait()
    except KeyboardInterrupt:
        proc.terminate()
        return proc.wait()


if __name__ == "__main__":
    raise SystemExit(main())
