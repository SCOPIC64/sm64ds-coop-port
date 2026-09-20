# Gameplay co-op networking

The lobby owns one nonblocking TCP star: clients send to the host and the host
relays to every other client. In addition to presence and chat, protocol v1
carries `STATE` snapshots containing the sender, monotonic sequence number,
area, character, fixed-point XYZ position, and facing angle.

Each instance keeps only the newest snapshot for a peer. Same-area snapshots
younger than five seconds are rendered through `hal_render_player_world`, the
normal SM64DS BMD player path. They are visual replicas and never enter the
local actor/physics lists; rendering temporarily seats the remote transform and
character resource, then restores every local player field.

This establishes movement-visible co-op without granting a remote socket write
access to local gameplay memory. Shared object authority, interaction events,
animation state, and level transitions are deliberately follow-up protocol
messages rather than guessed from transforms.

`lobby_state_probe` is ROM-free. Run one `host` process, then one
`join 127.0.0.1` process; both must print `gameplay snapshot relay: PASS`.
