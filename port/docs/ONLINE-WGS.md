# Online co-op rooms

The online menu uses a server profile instead of asking for an IP and port.
Rooms hold up to seven players, including their host. Movement snapshots and
character selections are forwarded from each player to the other room members.

## Play

1. The server owner starts WGS. The configured SERVER entry can automatically
   start its lobby, relay, Playit agent and Cloudflare HTTPS tunnel together.
2. Host: open the game, choose Host, select a save slot and character, and
   create an online lobby.
3. Share its six-character room code with friends.
4. Friend: choose Join, type that code, choose a character, and join the lobby.
5. Host: start when everyone is present. Each room receives a separate relay
   session key; an eighth player cannot enter a seven-player room.

Host and joining players need the same updated game. Opening a room does not
start the game until the host starts it and all current members acknowledge
the shared plan. Explicit online startup waits for peers and reports a timeout
instead of silently entering a solo session. Room heartbeats continue in-game.
Restarting the server loses existing rooms, so create a new one afterward.

Online rooms enter the castle directly on every peer and carry the selected
host save's 68-byte adventure record in the authenticated launch plan. They
use lockstep for all room sizes: the title and opening sequence previously
ran only on the host, which could exhaust a joining player's rollback ring
and silently leave both players in separate solo worlds. The opening sequence
still runs in solo play. Each joining game has a separate temporary save path;
the host continues to save to the selected file. When testing two windows on
one PC, keyboard and controller input belongs to the focused window.
After each entrance load, the camera target is restored to the local player's
actor as well as the local slot index. The original camera caches that actor
pointer; restoring only the index left joining players watching the host.

## Profile and server setup

Follow the [free WGS tunnel guide](https://github.com/SCOPIC64/sm64ds-coop-server/blob/main/deploy/NO-CARD-WGS.md)
or the [external host guide](https://github.com/SCOPIC64/sm64ds-coop-server/blob/main/deploy/SETUP.md).
Distribute `server-profile.json` beside the game executable. Its public relay
endpoint and public identity key are deployment data, outside the game source.
The game imports it into `%APPDATA%\SM64DS-Coop\server.txt` and protects the TXT
with Windows DPAPI for the current user. Existing profiles are read from that
encrypted TXT. Do not distribute one user's protected TXT to another user.

Profiles with `discovery_key` use the saved Playit UDP endpoint to request the
current HTTPS lobby origin. A fresh random challenge and RSA-2048/SHA-256
signature authenticate the reply before credentials are sent over HTTPS.
WGS updates the origin when a Quick Tunnel restarts; game users keep the same
profile. The server's private signing key is non-exportable in its Windows
account's key store. A changed relay tunnel or changed signing identity needs
an updated deployment profile. Static profiles use `lobby_url` and `relay_addr`
for conventional HTTPS hosts. Plans must match the configured relay endpoint.

The encryption key is managed by Windows, outside source and the executable.
Neither encrypted settings nor a custom cipher can make a network destination
undiscoverable to the player running the client. Public proxies keep normal
connections away from the server owner's home address; the public proxy
addresses themselves can be inspected. Only the WGS owner runs the tunnel
programs, and that PC must remain on while hosting. No agent secrets, private
keys, game saves, ROMs or private logs belong in the distributed client profile.
