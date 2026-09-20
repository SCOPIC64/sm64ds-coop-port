#pragma once

// Lobby transport: real TCP co-op networking (host + join by address).
//
// Roles: offline (nothing), hosting (listens, accepts joiners, relays),
// joined (one connection to a host). Star topology: joiners talk to the
// host, the host relays chat. No hardcoded addresses anywhere -- the join
// box ships blank and the host binds the wildcard.
//
// Protocol v1, line-based, UTF-8, LF-terminated, 220 chars max per line:
//   HELLO <name>            joiner introduces itself
//   WELCOME <name>          host replies with its own name
//   JOIN <name>             host tells everyone somebody arrived
//   LEAVE <name>            somebody left (or the socket died)
//   CHAT <sender> <text>    a chat line, relayed by the host
//
// The game sim is untouched: transport carries presence + chat only.
// Gameplay state sync is a later layer on top of these sockets.
//
// Threading: none. Everything is nonblocking and pumped from poll(),
// which the frame loop already calls every present.

namespace sm64ds::lobby {

/* default port when the address has none */
int default_port(void);

/* display name for this instance (from the front-end username) */
void set_name(const char *name);

/* UI events, delivered on the frame thread from poll(). Register once. */
struct NetEvents {
    /* an incoming chat line is ready to show */
    void (*text)(const char *user, const char *msg);
    /* a peer arrived (joined=1) or left (joined=0) */
    void (*peer)(const char *name, int joined);
};
void set_events(const NetEvents *ev);

void host_start(void);              /* listen; stays playable */
void join(const char *addr);        /* "ip", "ip:port", "localhost:port" */
void leave(void);                   /* drop everything, back to offline */
void send_chat(const char *user, const char *text);
void poll(void);
bool hosting(void);
bool joined(void);
/* 0 offline, 1 hosting, 2 joined */
int role(void);
/* human status for the LOBBY page, e.g. "Hosting :21330 (2)" */
void status_text(char *out, int cap);
/* lobby code: a typable short form of the host's LAN address + port.
   host_code writes e.g. "7F3K-9D2M-QA" while hosting, "" otherwise;
   decode_code turns a code (dashes/spaces/case ignored) back into
   "ip:port" for join(). A friend on the same network types the code
   instead of an address. */
int host_code(char *out, int cap);
int decode_code(const char *in, char *addr_out, int addr_cap);
/* connected peer names (host + joiners, not self) */
int peer_count(void);
const char *peer_name(int i);

}  // namespace sm64ds::lobby
