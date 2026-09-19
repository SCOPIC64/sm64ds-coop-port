# Online co-op launcher

tools/online_coop.py turns the PC port's networking environment contract into
a validated command line. It does not contain game assets or character data.

Relay play (no port forwarding):

    python tools/online_coop.py host --exe path\to\walk_window.exe --transport relay --relay relay.example.net --code MARIO64
    python tools/online_coop.py join --exe path\to\walk_window.exe --transport relay --relay relay.example.net --code MARIO64

Direct play requires UDP port 51765 to reach the host:

    python tools/online_coop.py host --exe path\to\walk_window.exe --transport direct
    python tools/online_coop.py join --exe path\to\walk_window.exe --transport direct --connect 203.0.113.10

For two copies on one computer, use --transport local; the joiner may select
--slot 1. Use --players 2 through --players 16 to size the session.

Add --dry-run to print the executable command and networking variables without
starting anything. The source-only relay service and its deployment guide are
under port/tools/relay/.
