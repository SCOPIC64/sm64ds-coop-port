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

## Names, chat and controls

Options -> Player -> Username edits your saved display name. Player Names
shows room names above characters; enable it in the same menu. Choose a name
before creating or joining a room. Player -> Character Colors edits each
character's hat, outfit, pants, gloves, shoes, skin, hair and emblem where the
DS texture supports that part. Yoshi has body, spikes, saddle, shoes and belly.
Select a part and enter six RGB hex digits, or change its red/green/blue rows.
Original Color restores one part; Reset Character restores that character.
Colors are saved per character and shared when joining the next lobby.

Options -> Controls has three binding slots for every action. Select an action,
select slot 1, 2 or 3, then Bind Keyboard or Bind Controller. Press a key, or
release the controller buttons and press the desired button. Any slot can hold
either input type, so DS A can use Y, U and controller A simultaneously. Esc
cancels capture; Clear Slot removes a binding. Profiles are saved in
`%APPDATA%\SM64DS-Coop\player.json` separately from the encrypted server TXT.

T opens chat in a lobby or game, Enter sends and Esc cancels. Chat input pauses
local keyboard/controller movement while typing. The chat action can also be
rebound. `/help` lists commands and `/players` shows seat numbers and roles.
Hosts can use `/mod NAME_OR_SEAT` and `/unmod NAME_OR_SEAT`. Hosts and appointed
moderators can use `/kick`, `/ban` and `/unban` with a player name or seat (unban
uses the banned name). Bans last until the room closes and use a saved install
identity; this is not an account-backed permanent ban system. Moderators cannot
remove the host or another moderator. Kicking during match startup is refused.

F11 switches to borderless fullscreen. The renderer uses the monitor's aspect
ratio to show more of the world horizontally, preserving character proportions.
F11 again restores the window. On the server PC, signed local relay discovery
avoids the public tunnel trip. Friends still connect through the public proxy;
the input buffer adapts to the slowest connected player.

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

When that same WGS relay runs on the player's PC at UDP port 41234, discovery
first checks loopback with a fresh challenge and the profile's public key.
Only a valid signed reply enables a local UDP route. The public relay in the
profile and launch plan stays pinned, so local players and internet friends
still share one room. Other computers automatically use the public tunnel.
The host sizes the shared input buffer from every joining player's measured
round trip; a verified local route lowers the minimum to one frame without
forcing internet friends to use an insufficient buffer.

The encryption key is managed by Windows, outside source and the executable.
Neither encrypted settings nor a custom cipher can make a network destination
undiscoverable to the player running the client. Public proxies keep normal
connections away from the server owner's home address; the public proxy
addresses themselves can be inspected. Only the WGS owner runs the tunnel
programs, and that PC must remain on while hosting. No agent secrets, private
keys, game saves, ROMs or private logs belong in the distributed client profile.
