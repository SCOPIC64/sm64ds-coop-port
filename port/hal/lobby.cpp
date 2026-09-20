// TCP lobby transport behind lobby.h: host + join by address, polled.
//
// Winsock rides LoadLibrary like everything else in this process (no
// static ws2_32 import: a static chain can plant mappings inside the
// fixed DS regions). All structs/constants below are spelled out so no
// winsock header -- and no link -- is needed.
#include "lobby.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace sm64ds::lobby {
namespace {

typedef uintptr_t NetSocket;
struct NetAddr {
    short family;
    unsigned short port;
    unsigned long ip;
    char zero[8];
};
struct NetFds {
    unsigned count;
    NetSocket arr[64];
};
struct NetTimeout {
    long sec;
    long usec;
};
struct NetAddrInfo {
    int flags;
    int family;
    int socktype;
    int protocol;
    size_t addrlen;
    char *canon;
    struct sockaddr *addr;
    struct NetAddrInfo *next;
};

enum {
    NET_AF_INET = 2,
    NET_SOCK_STREAM = 1,
    NET_TCP = 6,
    NET_SOL_SOCKET = 0xffff,
    NET_SO_ERROR = 0x1007,
    NET_FIONBIO = 0x8004667e,
    NET_WOULDBLOCK = 10035,
    NET_ISCONN = 10056,
};
#define NET_INVALID ((NetSocket)~(uintptr_t)0)

struct NetApi {
    /* __stdcall = WSAAPI: winsock entry points, no winsock headers here */
    int (__stdcall *startup)(unsigned short, void *);
    int (__stdcall *cleanup)(void);
    NetSocket (__stdcall *socket)(int, int, int);
    int (__stdcall *closesocket)(NetSocket);
    int (__stdcall *bind)(NetSocket, const void *, int);
    int (__stdcall *listen)(NetSocket, int);
    NetSocket (__stdcall *accept)(NetSocket, void *, int *);
    int (__stdcall *connect)(NetSocket, const void *, int);
    int (__stdcall *send)(NetSocket, const char *, int, int);
    int (__stdcall *recv)(NetSocket, char *, int, int);
    int (__stdcall *select)(int, NetFds *, NetFds *, NetFds *,
                            const NetTimeout *);
    int (__stdcall *ioctl)(NetSocket, long, unsigned long *);
    int (__stdcall *getaddrinfo)(const char *, const char *, const void *,
                                 NetAddrInfo **);
    void (__stdcall *freeaddrinfo)(NetAddrInfo *);
    int (__stdcall *getsockopt)(NetSocket, int, int, char *, int *);
    int (__stdcall *geterror)(void);
};
static NetApi N;
static int net_ok;

static unsigned short net_htons(unsigned short v)
{
    return (unsigned short)((v << 8) | (v >> 8));
}

static int net_load(void)
{
    if (net_ok) return 1;
    HMODULE w = LoadLibraryA("ws2_32.dll");
    if (!w) {
        std::printf("[net] ws2_32 missing, staying offline\n");
        return 0;
    }
#define NETSYM(nm, str)                                                  \
    N.nm = (decltype(N.nm))GetProcAddress(w, str);                       \
    if (!N.nm) {                                                         \
        std::printf("[net] %s missing, staying offline\n", str);         \
        return 0;                                                        \
    }
    NETSYM(startup, "WSAStartup");
    NETSYM(cleanup, "WSACleanup");
    NETSYM(socket, "socket");
    NETSYM(closesocket, "closesocket");
    NETSYM(bind, "bind");
    NETSYM(listen, "listen");
    NETSYM(accept, "accept");
    NETSYM(connect, "connect");
    NETSYM(send, "send");
    NETSYM(recv, "recv");
    NETSYM(select, "select");
    NETSYM(ioctl, "ioctlsocket");
    NETSYM(getaddrinfo, "getaddrinfo");
    NETSYM(freeaddrinfo, "freeaddrinfo");
    NETSYM(getsockopt, "getsockopt");
    NETSYM(geterror, "WSAGetLastError");
#undef NETSYM
    /* WSAStartup(2.2): version word 0x0202, high byte major */
    {
        char blob[64];
        memset(blob, 0, sizeof blob);
        if (N.startup(0x0202, blob) != 0) {
            std::printf("[net] WSAStartup failed, staying offline\n");
            return 0;
        }
    }
    net_ok = 1;
    return 1;
}

/* ---- peers ------------------------------------------------------------ */

enum { MAX_PEERS = 8, NAME_LEN = 16, LINE_MAX = 220 };

struct Peer {
    NetSocket s;
    int live;          /* slot in use */
    int connected;     /* hello completed (joiner: welcome received) */
    int connecting;    /* join in progress (nonblocking connect) */
    char name[NAME_LEN];
    char rbuf[LINE_MAX + 2];
    int rlen;
};

static NetSocket listen_sock = NET_INVALID;
static Peer peers[MAX_PEERS];
static int role_state; /* 0 offline, 1 hosting, 2 joined */
static char self_name[NAME_LEN] = "Player";
static const NetEvents *events;
/* visible roster: socket peers (host view) plus relayed names (joiner
   view share it). Small and deduplicated; the LOBBY page reads it. */
static char roster[MAX_PEERS][NAME_LEN];
static int roster_n;

static void roster_add(const char *name)
{
    if (!name || !name[0]) return;
    for (int i = 0; i < roster_n; ++i)
        if (!strcmp(roster[i], name)) return;
    if (roster_n >= MAX_PEERS) return;
    snprintf(roster[roster_n], NAME_LEN, "%s", name);
    ++roster_n;
}

static void roster_drop(const char *name)
{
    for (int i = 0; i < roster_n; ++i)
        if (!strcmp(roster[i], name)) {
            for (int j = i; j + 1 < roster_n; ++j)
                snprintf(roster[j], NAME_LEN, "%s", roster[j + 1]);
            --roster_n;
            return;
        }
}

static void ev_text(const char *user, const char *msg)
{
    if (events && events->text) events->text(user, msg);
}

static void ev_peer(const char *name, int joined)
{
    if (events && events->peer) events->peer(name, joined);
}

static void ev_state(const char *name, unsigned sequence, int area,
                     int character, int x, int y, int z, int yaw)
{
    if (events && events->state)
        events->state(name, sequence, area, character, x, y, z, yaw);
}

static void clean_name(const char *src, char *dst)
{
    int i = 0;
    for (; src[i] && i < NAME_LEN - 1; ++i) {
        unsigned char ch = (unsigned char)src[i];
        dst[i] = (ch < 32 || ch > 126 || ch == ' ') ? '_' : (char)ch;
    }
    dst[i] = 0;
    if (!dst[0]) {
        dst[0] = 'P';
        dst[1] = 'l';
        dst[2] = 'a';
        dst[3] = 'y';
        dst[4] = 'e';
        dst[5] = 'r';
        dst[6] = 0;
    }
}

static void peer_close(Peer *p)
{
    if (p->s != NET_INVALID) N.closesocket(p->s);
    p->s = NET_INVALID;
    p->live = 0;
    p->connected = 0;
    p->connecting = 0;
    p->name[0] = 0;
    p->rlen = 0;
}

static Peer *peer_alloc(void)
{
    for (int i = 0; i < MAX_PEERS; ++i)
        if (!peers[i].live) {
            peers[i].live = 1;
            peers[i].s = NET_INVALID;
            peers[i].connected = 0;
            peers[i].connecting = 0;
            peers[i].name[0] = 0;
            peers[i].rlen = 0;
            return &peers[i];
        }
    return 0;
}

/* best-effort send of one LF-terminated line */
static void peer_send(Peer *p, const char *line)
{
    char buf[LINE_MAX + 2];
    int len = 0;
    while (line[len] && len < LINE_MAX) {
        buf[len] = line[len];
        ++len;
    }
    buf[len++] = '\n';
    int off = 0;
    while (off < len) {
        int r = N.send(p->s, buf + off, len - off, 0);
        if (r <= 0) {
            int e = N.geterror();
            if (r < 0 && (e == NET_WOULDBLOCK || e == NET_ISCONN)) break;
            break;
        }
        off += r;
    }
}

static void host_broadcast(const char *line, Peer *skip)
{
    for (int i = 0; i < MAX_PEERS; ++i)
        if (peers[i].live && peers[i].connected && &peers[i] != skip) {
            char msg[LINE_MAX + 2];
            snprintf(msg, sizeof msg, "%s", line);
            peer_send(&peers[i], msg);
        }
}

static void drop_peer(Peer *p, const char *why)
{
    char name[NAME_LEN];
    snprintf(name, sizeof name, "%s", p->name[0] ? p->name : "peer");
    std::printf("[net] drop %s (%s)\n", name, why);
    int was_named = p->connected && p->name[0];
    peer_close(p);
    if (role_state == 1) {
        /* hosting: everyone else hears about it */
        if (was_named) {
            char msg[64];
            snprintf(msg, sizeof msg, "LEAVE %s", name);
            host_broadcast(msg, 0);
            roster_drop(name);
            ev_peer(name, 0);
        }
    } else if (role_state == 2) {
        /* joined: losing the host ends the session */
        role_state = 0;
        roster_drop(name);
        ev_peer(name, 0);
        std::printf("[net] host gone, offline\n");
    }
}

/* ---- inbound lines ------------------------------------------------------ */

static void handle_line(Peer *p, char *line)
{
    if (!strncmp(line, "HELLO ", 6)) {
        if (role_state != 1) return;
        clean_name(line + 6, p->name);
        p->connected = 1;
        std::printf("[net] %s joined\n", p->name);
        {
            char msg[64];
            snprintf(msg, sizeof msg, "WELCOME %s", self_name);
            peer_send(p, msg);
            snprintf(msg, sizeof msg, "JOIN %s", p->name);
            host_broadcast(msg, p);
        }
        roster_add(p->name);
        ev_peer(p->name, 1);
    } else if (!strncmp(line, "WELCOME ", 8)) {
        if (role_state != 2) return;
        clean_name(line + 8, p->name);
        p->connected = 1;
        std::printf("[net] welcomed by %s\n", p->name);
        roster_add(p->name);
        ev_peer(p->name, 1);
    } else if (!strncmp(line, "JOIN ", 5)) {
        char name[NAME_LEN];
        clean_name(line + 5, name);
        std::printf("[net] %s arrived\n", name);
        roster_add(name);
        ev_peer(name, 1);
    } else if (!strncmp(line, "LEAVE ", 6)) {
        char name[NAME_LEN];
        clean_name(line + 6, name);
        if (role_state == 1 && p->connected) {
            /* a joiner leaving through the host: relay + drop */
            char msg[64];
            snprintf(msg, sizeof msg, "LEAVE %s", name);
            host_broadcast(msg, p);
            roster_drop(name);
            ev_peer(name, 0);
            if (!strcmp(p->name, name)) peer_close(p);
        } else {
            drop_peer(p, "left");
        }
    } else if (!strncmp(line, "CHAT ", 5)) {
        char *sp = strchr(line + 5, ' ');
        if (!sp) return;
        *sp = 0;
        char from[NAME_LEN], text[LINE_MAX];
        clean_name(line + 5, from);
        snprintf(text, sizeof text, "%s", sp + 1);
        std::printf("[net] <%s> %s\n", from, text);
        if (role_state == 1 && p->connected) {
            /* relay to everyone else with the origin attached */
            char msg[LINE_MAX + 2];
            snprintf(msg, sizeof msg, "CHAT %s %s", from, text);
            host_broadcast(msg, p);
        }
        ev_text(from, text);
    } else if (!strncmp(line, "STATE ", 6)) {
        char from[NAME_LEN];
        unsigned sequence;
        int area, character, x, y, z, yaw;
        if (sscanf(line + 6, "%15s %u %d %d %d %d %d %d", from,
                   &sequence, &area, &character, &x, &y, &z, &yaw) != 8)
            return;
        clean_name(from, from);
        if (role_state == 1 && p->connected) {
            /* The socket identity wins over a claimed sender name. */
            snprintf(from, sizeof from, "%s", p->name);
            char msg[LINE_MAX + 2];
            snprintf(msg, sizeof msg, "STATE %s %u %d %d %d %d %d %d",
                     from, sequence, area, character, x, y, z, yaw);
            host_broadcast(msg, p);
        }
        ev_state(from, sequence, area, character, x, y, z, yaw);
    }
    /* unknown lines are ignored: forward-compat by design */
}

static void peer_pump(Peer *p)
{
    char tmp[128];
    int r = N.recv(p->s, tmp, sizeof tmp - 1, 0);
    if (r == 0) {
        drop_peer(p, "closed");
        return;
    }
    if (r < 0) {
        int e = N.geterror();
        if (e == NET_WOULDBLOCK) return;
        drop_peer(p, "recv error");
        return;
    }
    for (int i = 0; i < r; ++i) {
        char ch = tmp[i];
        if (ch == '\r') continue;
        if (ch == '\n') {
            p->rbuf[p->rlen] = 0;
            if (p->rlen > 0) handle_line(p, p->rbuf);
            /* the handler may have closed the peer (LEAVE) */
            if (!p->live) return;
            p->rlen = 0;
            continue;
        }
        if (p->rlen < LINE_MAX) p->rbuf[p->rlen++] = ch;
        /* overlong lines: keep consuming to the newline, drop the excess */
    }
}

}  // namespace

/* ---- public API ----------------------------------------------------------- */

int default_port(void) { return 21330; }

void set_name(const char *name)
{
    if (name) clean_name(name, self_name);
}

void set_events(const NetEvents *ev) { events = ev; }

void host_start(void)
{
    if (role_state == 1) return;
    if (!net_load()) return;
    if (role_state != 0) leave();
    NetSocket s = N.socket(NET_AF_INET, NET_SOCK_STREAM, NET_TCP);
    if (s == NET_INVALID) {
        std::printf("[net] socket failed\n");
        return;
    }
    unsigned long one = 1;
    N.ioctl(s, NET_FIONBIO, &one);
    NetAddr a;
    memset(&a, 0, sizeof a);
    a.family = NET_AF_INET;
    a.port = net_htons((unsigned short)default_port());
    a.ip = 0; /* wildcard: no address is ever hardcoded */
    if (N.bind(s, &a, sizeof a) != 0 || N.listen(s, 4) != 0) {
        std::printf("[net] listen :%d failed (%d)\n", default_port(),
                    N.geterror());
        N.closesocket(s);
        return;
    }
    listen_sock = s;
    role_state = 1;
    std::printf("[net] hosting on :%d as %s\n", default_port(), self_name);
}

void join(const char *addr)
{
    char host[128] = {0};
    int port = default_port();
    if (role_state != 0) leave();
    if (!net_load()) return;
    if (addr) {
        /* "host", "host:port" -- split on the last colon when the tail
           is numeric, else the whole thing is the host. No defaults
           beyond the port; a blank box stays blank upstream. */
        const char *colon = strrchr(addr, ':');
        if (colon && colon[1]) {
            char *end = 0;
            long pv = strtol(colon + 1, &end, 10);
            if (end && !*end && pv > 0 && pv < 65536) {
                size_t n = (size_t)(colon - addr);
                if (n >= sizeof host) n = sizeof host - 1;
                memcpy(host, addr, n);
                host[n] = 0;
                port = (int)pv;
            } else {
                snprintf(host, sizeof host, "%s", addr);
            }
        } else {
            snprintf(host, sizeof host, "%s", addr);
        }
    }
    if (!host[0]) {
        std::printf("[net] join: blank address\n");
        return;
    }
    char service[16];
    snprintf(service, sizeof service, "%d", port);
    NetAddrInfo *res = 0;
    if (N.getaddrinfo(host, service, 0, &res) != 0 || !res) {
        std::printf("[net] resolve failed: %s\n", host);
        return;
    }
    NetSocket s = N.socket(NET_AF_INET, NET_SOCK_STREAM, NET_TCP);
    if (s == NET_INVALID) {
        N.freeaddrinfo(res);
        return;
    }
    unsigned long one = 1;
    N.ioctl(s, NET_FIONBIO, &one);
    int r = N.connect(s, res->addr, (int)res->addrlen);
    N.freeaddrinfo(res);
    if (r != 0) {
        int e = N.geterror();
        /* WSAEWOULDBLOCK (10035): connect in progress, poll completes it */
        if (e != NET_WOULDBLOCK) {
            std::printf("[net] connect failed (%d)\n", e);
            N.closesocket(s);
            return;
        }
    }
    Peer *p = peer_alloc();
    if (!p) {
        N.closesocket(s);
        return;
    }
    p->s = s;
    p->connecting = 1;
    snprintf(p->name, sizeof p->name, "%s", host);
    role_state = 2;
    std::printf("[net] joining %s:%d ...\n", host, port);
}

void leave(void)
{
    for (int i = 0; i < MAX_PEERS; ++i)
        if (peers[i].live) {
            if (peers[i].connected) {
                char msg[64];
                snprintf(msg, sizeof msg, "LEAVE %s", self_name);
                peer_send(&peers[i], msg);
            }
            peer_close(&peers[i]);
        }
    roster_n = 0;
    if (listen_sock != NET_INVALID) {
        N.closesocket(listen_sock);
        listen_sock = NET_INVALID;
    }
    if (role_state != 0) std::printf("[net] offline\n");
    role_state = 0;
}

void send_chat(const char *user, const char *text)
{
    if (!user) user = "?";
    if (!text) text = "";
    std::printf("[lobby] <%s> %s\n", user, text);
    if (role_state == 0) return;
    char msg[LINE_MAX + 2];
    if (role_state == 1) {
        snprintf(msg, sizeof msg, "CHAT %s %s", user, text);
        host_broadcast(msg, 0);
    } else {
        snprintf(msg, sizeof msg, "CHAT %s %s", user, text);
        for (int i = 0; i < MAX_PEERS; ++i)
            if (peers[i].live && !peers[i].connecting)
                peer_send(&peers[i], msg);
    }
}

void send_state(unsigned sequence, int area, int character,
                int x, int y, int z, int yaw)
{
    if (role_state == 0) return;
    char msg[LINE_MAX + 2];
    snprintf(msg, sizeof msg, "STATE %s %u %d %d %d %d %d %d",
             self_name, sequence, area, character, x, y, z, yaw);
    if (role_state == 1) {
        host_broadcast(msg, 0);
    } else {
        for (int i = 0; i < MAX_PEERS; ++i)
            if (peers[i].live && peers[i].connected && !peers[i].connecting)
                peer_send(&peers[i], msg);
    }
}

void poll(void)
{
    if (!net_ok) return;
    NetTimeout zero = {0, 0};
    /* new joiners */
    if (listen_sock != NET_INVALID) {
        for (;;) {
            NetFds rfds;
            rfds.count = 1;
            rfds.arr[0] = listen_sock;
            if (N.select(0, &rfds, 0, 0, &zero) <= 0) break;
            NetSocket s = N.accept(listen_sock, 0, 0);
            if (s == NET_INVALID) break;
            unsigned long one = 1;
            N.ioctl(s, NET_FIONBIO, &one);
            Peer *p = peer_alloc();
            if (!p) {
                N.closesocket(s);
                break;
            }
            p->s = s;
            std::printf("[net] incoming connection\n");
        }
    }
    for (int i = 0; i < MAX_PEERS; ++i) {
        Peer *p = &peers[i];
        if (!p->live) continue;
        if (p->connecting) {
            /* join in progress: writable (+ no error) means connected */
            NetFds wfds;
            wfds.count = 1;
            wfds.arr[0] = p->s;
            if (N.select(0, 0, &wfds, 0, &zero) <= 0) continue;
            int err = 0, len = sizeof err;
            if (N.getsockopt(p->s, NET_SOL_SOCKET, NET_SO_ERROR,
                             (char *)&err, &len) != 0 ||
                err != 0) {
                std::printf("[net] connect failed (%d)\n", err);
                peer_close(p);
                if (role_state == 2) role_state = 0;
                continue;
            }
            p->connecting = 0;
            {
                char msg[64];
                snprintf(msg, sizeof msg, "HELLO %s", self_name);
                peer_send(p, msg);
            }
            std::printf("[net] connected, said hello\n");
            continue;
        }
        {
            NetFds rfds;
            rfds.count = 1;
            rfds.arr[0] = p->s;
            if (N.select(0, &rfds, 0, 0, &zero) <= 0) continue;
            peer_pump(p);
        }
    }
}

bool hosting(void) { return role_state == 1; }

bool joined(void) { return role_state == 2; }

int role(void) { return role_state; }

void status_text(char *out, int cap)
{
    if (role_state == 1)
        snprintf(out, cap, "Hosting :%d (%d)", default_port(),
                 roster_n + 1);
    else if (role_state == 2)
        snprintf(out, cap, "Joined (%d)", roster_n + 1);
    else
        snprintf(out, cap, "Offline");
}

int peer_count(void) { return roster_n; }

const char *peer_name(int i)
{
    if (i < 0 || i >= roster_n) return "";
    return roster[i];
}

}  // namespace sm64ds::lobby
