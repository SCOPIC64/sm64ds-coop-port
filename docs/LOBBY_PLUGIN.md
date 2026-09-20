# Lobby plugin (netplay seam)

The menu, chat box, username, and character/color picks already exist. What
is missing is transport, and this file says exactly where it goes so a
later contributor (or the repo owner) can implement lobbies without
redesigning anything.

## Contract

`port/hal/lobby.h` is the whole contract:

```c++
sm64ds::lobby::host_start();   // open a local lobby
sm64ds::lobby::leave();        // close it
sm64ds::lobby::send_chat(user, text);  // broadcast a chat line
sm64ds::lobby::poll();         // pump incoming traffic each frame
sm64ds::lobby::hosting();      // nonzero while hosting
```

Today `port/hal/lobby.cpp` implements it as real TCP: `host_start` binds
the wildcard on port 21330, `join` dials a blank-by-default address box
(`"ip"`, `"ip:port"`, `"localhost:port"`), `poll()` accepts/pumps/relays
every frame, and incoming chat lands in the same box local typing uses
(via a no-rebroadcast path, so relayed lines never echo-storm). No menu,
game-tick, or render changes were needed.

Lobby codes: `host_code` encodes the host's LAN IPv4 + port as 48 bits
of Crockford base32 (`XXXX-XXXX-XX`); `decode_code` turns it back into
`ip:port` inside `join()`, so the box (and any future caller) accepts a
code or an address interchangeably. The LAN address comes from a
no-traffic UDP route lookup against TEST-NET-1, never from a hardcoded
or public-IP service; loopback is refused as useless to a friend.

## Suggested shape (when you build it)

- Host: `host_start` binds a socket, `poll` accepts peers and steps them.
- State sync: the guest already ticks the same ROM code; the minimum viable
  sync is player transform + character + animation id at 10-20 Hz, with the
  remote peer rendered as a second Player (the actor registry already knows
  how to build one; the camera follows the local).
- Chat: route `send_chat` over the socket; incoming lines land in the same
  `chat_add` the local box uses.
- Character/color: broadcast the MODS picks on join so peers agree.
- Lua: expose the same four verbs as a `lobby.*` table next to the
  `sm64ds.*` table in `port/hal/mod_loader.cpp` so mods written now keep
  working.

## What NOT to do

- Do not put sockets in the game tick: `poll()` runs on the frame loop
  beside `sdat_host_tick`, never inside `port_actor_tick`.
- Do not sync by sending inputs: floating point and frame pacing differ
  across machines; send results (transforms), not causes.
