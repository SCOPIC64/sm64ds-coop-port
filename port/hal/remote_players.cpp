// Remote-player puppets behind remote_players.h: spawned Player actors
// that render but never simulate. Each frame the owner overwrites the
// transform from POS lines (with a half-step lerp so 15 Hz reads
// smooth); anims switch through the same ensure+SetAnim path the live
// character switch uses, then advance on the frame tick.
#include "remote_players.h"

#include <cstdio>
#include <cstring>
#include <excpt.h>   /* EXCEPTION_EXECUTE_HANDLER for the puppet guards */

#include "lobby.h"

struct SharedFilePtr;

extern "C" {
void *_ZN6PlayerC1Ev(void *self);
void *_ZN9ActorBasenwEj(unsigned size);
int hal_player_init_resources(void *p);
void hal_player_set_real_character(void *p, unsigned chr);
void hal_render_player_world(void *p);
void Player_AdvanceAnims(char *p);
void _ZN6Player7SetAnimEji5Fix12IiEj(void *c, unsigned a, int b, int f,
                                     unsigned d);
int _ZN6Player6IsAnimEj(void *c, unsigned a);
unsigned _ZNK6Player14GetBodyModelIDEjb(void *self, unsigned a, int b);
void _ZN9Animation8LoadFileER13SharedFilePtr(SharedFilePtr &f);
void *_ZN13SharedFilePtr8LoadFileEv(SharedFilePtr *self);
extern int data_0209f394[];    /* per-player Actor* */
extern int data_0209fc5c[];    /* per-player "this slot is live" */
extern int data_0209caa0[];    /* save block: [0x41] is the character */
extern void *data_ov002_020ff480[];
extern void *data_ov002_020ff2f0[];
}

namespace sm64ds::remote {
namespace {

enum { MAX_PUPPETS = 3, POS_TIMEOUT = 600, BUMP_DIST = 120,
       BUMP_COOLDOWN = 150 };

struct Puppet {
    char name[16];
    char *actor;
    int slot;   /* 1..3 */
    int chr;
    int anim;
    int last;   /* tick of the last POS */
    int tx, ty, tz;  /* lerp target, Fix12 */
};

static Puppet pups[MAX_PUPPETS];
static int tick;
static int last_role;
static int last_bump_tick = -100000;
static char bump_name[16];

/* same garbage rule as the live switch: a null filePtr or a DS-RAM
   address in the slot means "never loaded on host" -- load for real */
static void *puppet_ensure_file(void *slot)
{
    void *fp = *(void **)((char *)slot + 4);
    if (fp == 0 ||
        ((unsigned)(size_t)fp >= 0x02000000u &&
         (unsigned)(size_t)fp < 0x02800000u)) {
        *(unsigned char *)((char *)slot + 2) = 0;
        *(void **)((char *)slot + 4) = 0;
        _ZN9Animation8LoadFileER13SharedFilePtr(*(SharedFilePtr *)slot);
        fp = *(void **)((char *)slot + 4);
    }
    return fp;
}

/* character switch that does not touch the save block: SetRealCharacter
   stamps data_0209caa0[0x41] as a side effect, which belongs to the
   local player alone */
static void puppet_set_character(char *p, unsigned chr)
{
    unsigned char keep = *((unsigned char *)data_0209caa0 + 0x41);
    hal_player_set_real_character(p, chr);
    *((unsigned char *)data_0209caa0 + 0x41) = keep;
}

/* anim switch for (chr, anim): ensure the body + texture files the way
   the live switch does, then force SetAnim's slow path so the frame
   count reseeds instead of dividing by zero */
static void puppet_set_anim(char *p, unsigned chr, unsigned anim)
{
    if (anim > 0x1ff) return;
    puppet_ensure_file(data_ov002_020ff480[(chr + anim * 4)]);
    {
        unsigned sl = anim * 4;
        if ((int)sl >= 0x60) sl = 0x60;
        sl += chr;
        void *tex = data_ov002_020ff2f0[sl];
        void *fp = *(void **)((char *)tex + 4);
        if (fp == 0 ||
            ((unsigned)(size_t)fp >= 0x02000000u &&
             (unsigned)(size_t)fp < 0x02800000u)) {
            *(unsigned char *)((char *)tex + 2) = 0;
            *(void **)((char *)tex + 4) = 0;
            _ZN13SharedFilePtr8LoadFileEv((SharedFilePtr *)tex);
        }
    }
    unsigned m1 = _ZNK6Player14GetBodyModelIDEjb(p, chr, 0);
    *(void **)(*(char **)(p + m1 * 4 + 0xdc) + 0x60) = 0;
    _ZN6Player7SetAnimEji5Fix12IiEj(p, anim, 0, 0x1000, 0);
}

static Puppet *find_pup(const char *name)
{
    for (int i = 0; i < MAX_PUPPETS; ++i)
        if (pups[i].actor && !strcmp(pups[i].name, name)) return &pups[i];
    return 0;
}

/* Puppets are cosmetic: no puppet fault may ever kill the sim. Every
   touch of game/render code goes through this; on a fault the puppet
   is parked (and after 3 faults the name is not respawned until the
   next session) and the game marches on. Plain data only in here, so
   __try is safe under /EHsc. */
static int pup_faults;
static char no_spawn[8][16];

template <typename F> static int guarded(F f)
{
    __try {
        f();
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static void blacklist(const char *name)
{
    if (++pup_faults > 100) return;
    for (int i = 0; i < 8; ++i)
        if (!no_spawn[i][0]) {
            snprintf(no_spawn[i], sizeof no_spawn[i], "%s", name);
            return;
        }
}

static int blacklisted(const char *name)
{
    int n = 0;
    for (int i = 0; i < 8; ++i)
        if (no_spawn[i][0] && !strcmp(no_spawn[i], name)) ++n;
    return n >= 3;
}

static int bad_chr = -1, bad_anim = -1;

static void teardown(Puppet *pu, const char *why)
{
    if (!pu->actor) return;
    std::printf("[net] puppet %s %s\n", pu->name, why);
    data_0209fc5c[pu->slot] = 0;
    data_0209f394[pu->slot] = 0;
    /* the 0x768-byte actor is abandoned, not freed: no delete path
       exists for these, and one per session is noise */
    pu->actor = 0;
    pu->name[0] = 0;
}

static Puppet *spawn_pup(const char *name, int chr)
{
    if (blacklisted(name)) return 0;
    int slot = 0;
    for (int s = 1; s <= MAX_PUPPETS; ++s)
        if (data_0209f394[s] == 0) {
            slot = s;
            break;
        }
    if (!slot) {
        std::printf("[net] puppet %s: no free player slot\n", name);
        return 0;
    }
    char *p = (char *)_ZN9ActorBasenwEj(0x800);
    if (!p) return 0;
    _ZN6PlayerC1Ev(p);
    if (hal_player_init_resources(p) != 1) return 0;
    *(unsigned char *)(p + 0x6d8) = (unsigned char)slot;
    *(int *)(p + 0x358) = 0;          /* held object: none */
    *(unsigned char *)(p + 0x713) = 0; /* body collision: off (ghost) */
    data_0209f394[slot] = (int)(size_t)p;
    data_0209fc5c[slot] = 1;
    if (!guarded([&]() { puppet_set_character(p, (unsigned)chr); })) {
        std::printf("[net] puppet %s: character fault contained\n",
                    name);
        blacklist(name);
        data_0209f394[slot] = 0;
        data_0209fc5c[slot] = 0;
        return 0;
    }
    Puppet *pu = 0;
    for (int i = 0; i < MAX_PUPPETS; ++i)
        if (!pups[i].actor) {
            pu = &pups[i];
            break;
        }
    if (!pu) {
        data_0209f394[slot] = 0;
        data_0209fc5c[slot] = 0;
        return 0;
    }
    snprintf(pu->name, sizeof pu->name, "%s", name);
    pu->actor = p;
    pu->slot = slot;
    pu->chr = chr;
    pu->anim = -1;
    pu->last = tick;
    std::printf("[net] puppet %s spawned in slot %d as %d\n", name, slot,
                chr);
    return pu;
}

}  // namespace

void net_pos(const char *name, int x, int y, int z, int yaw, int chr,
             int anim)
{
    if (!name || !name[0]) return;
    Puppet *pu = find_pup(name);
    if (!pu) {
        pu = spawn_pup(name, chr);
        if (!pu) return;
        pu->tx = *(int *)(pu->actor + 0x5c);
        pu->ty = *(int *)(pu->actor + 0x60);
        pu->tz = *(int *)(pu->actor + 0x64);
    }
    if (chr != pu->chr && chr >= 0 && chr <= 3) {
        if (!guarded([&]() {
                puppet_set_character(pu->actor, (unsigned)chr);
            })) {
            std::printf("[net] puppet %s: character fault contained\n",
                        name);
            blacklist(name);
            teardown(pu, "fault");
            return;
        }
        pu->chr = chr;
        pu->anim = -1;
    }
    /* far target (arrival, door) snaps; the rest lerps in send_tick */
    {
        int cx = (x << 12) - pu->tx, cy = (y << 12) - pu->ty,
            cz = (z << 12) - pu->tz;
        if (cx > 2000 * 4096 || cx < -2000 * 4096 ||
            cy > 2000 * 4096 || cy < -2000 * 4096 ||
            cz > 2000 * 4096 || cz < -2000 * 4096) {
            pu->tx = x << 12;
            pu->ty = y << 12;
            pu->tz = z << 12;
            *(int *)(pu->actor + 0x5c) = pu->tx;
            *(int *)(pu->actor + 0x60) = pu->ty;
            *(int *)(pu->actor + 0x64) = pu->tz;
        } else {
            pu->tx = x << 12;
            pu->ty = y << 12;
            pu->tz = z << 12;
        }
    }
    *(short *)(pu->actor + 0x8e) = (short)yaw;
    if (anim != pu->anim && anim >= 0 && anim <= 0x1ff &&
        !(chr == bad_chr && anim == bad_anim)) {
        if (!guarded([&]() {
                puppet_set_anim(pu->actor, (unsigned)pu->chr,
                                (unsigned)anim);
            })) {
            std::printf("[net] puppet %s: anim %d fault contained\n",
                        name, anim);
            bad_chr = chr;
            bad_anim = anim;
            blacklist(name);
            teardown(pu, "fault");
            return;
        }
        pu->anim = anim;
    }
    pu->last = tick;
}

void drop(const char *name)
{
    Puppet *pu = find_pup(name);
    if (pu) teardown(pu, "left");
}

static int local_anim(char *c, int prev)
{
    if (prev >= 0 && _ZN6Player6IsAnimEj(c, (unsigned)prev)) return prev;
    for (unsigned a = 0; a < 0x200; ++a)
        if (_ZN6Player6IsAnimEj(c, a)) return (int)a;
    return prev;
}

void send_tick(void *local_player, int role)
{
    ++tick;
    if (role == 0) {
        if (last_role != 0) {
            for (int i = 0; i < MAX_PUPPETS; ++i)
                if (pups[i].actor) teardown(&pups[i], "offline");
            /* fresh session, fresh chances */
            memset(no_spawn, 0, sizeof no_spawn);
            pup_faults = 0;
            bad_chr = bad_anim = -1;
        }
        last_role = 0;
        return;
    }
    last_role = role;
    for (int i = 0; i < MAX_PUPPETS; ++i) {
        if (!pups[i].actor) continue;
        if (tick - pups[i].last > POS_TIMEOUT) {
            teardown(&pups[i], "timed out");
            continue;
        }
        /* lerp toward the target so 15 Hz reads smooth */
        char *c = pups[i].actor;
        int px = *(int *)(c + 0x5c), py = *(int *)(c + 0x60),
            pz = *(int *)(c + 0x64);
        *(int *)(c + 0x5c) = px + ((pups[i].tx - px) >> 1);
        *(int *)(c + 0x60) = py + ((pups[i].ty - py) >> 1);
        *(int *)(c + 0x64) = pz + ((pups[i].tz - pz) >> 1);
        if (!guarded([&]() { Player_AdvanceAnims(c); })) {
            std::printf("[net] puppet %s: advance fault contained\n",
                        pups[i].name);
            blacklist(pups[i].name);
            teardown(&pups[i], "fault");
            continue;
        }
    }
    if (!local_player || (tick & 1)) return;
    /* ~15 Hz POS out: on change, plus a 2 s heartbeat for late joiners */
    static int lx, ly, lz, lyaw, lchr, lanim = -1, last_sent;
    char *c = (char *)local_player;
    int x = *(int *)(c + 0x5c) >> 12, y = *(int *)(c + 0x60) >> 12,
        z = *(int *)(c + 0x64) >> 12;
    int yaw = *(short *)(c + 0x8e);
    int chr = *(unsigned char *)(c + 0x6d9) & 3;
    int anim = local_anim(c, lanim);
    if (x != lx || y != ly || z != lz || yaw != lyaw || chr != lchr ||
        anim != lanim || tick - last_sent > 60) {
        lx = x;
        ly = y;
        lz = z;
        lyaw = yaw;
        lchr = chr;
        lanim = anim;
        last_sent = tick;
        sm64ds::lobby::send_pos(x, y, z, yaw, chr, anim);
    }
}

void draw(void)
{
    for (int i = 0; i < MAX_PUPPETS; ++i) {
        if (!pups[i].actor) continue;
        char *c = pups[i].actor;
        if (!guarded([&]() { hal_render_player_world(c); })) {
            std::printf("[net] puppet %s: draw fault contained\n",
                        pups[i].name);
            blacklist(pups[i].name);
            teardown(&pups[i], "fault");
        }
    }
}

const char *bump(int x, int y, int z)
{
    if (tick - last_bump_tick < BUMP_COOLDOWN) return 0;
    for (int i = 0; i < MAX_PUPPETS; ++i) {
        if (!pups[i].actor) continue;
        char *c = pups[i].actor;
        int px = *(int *)(c + 0x5c) >> 12, py = *(int *)(c + 0x60) >> 12,
            pz = *(int *)(c + 0x64) >> 12;
        int dx = px - x, dy = py - y, dz = pz - z;
        if (dx < BUMP_DIST && dx > -BUMP_DIST && dz < BUMP_DIST &&
            dz > -BUMP_DIST && dy < 150 && dy > -150) {
            last_bump_tick = tick;
            snprintf(bump_name, sizeof bump_name, "%s", pups[i].name);
            return bump_name;
        }
    }
    return 0;
}

int count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_PUPPETS; ++i)
        if (pups[i].actor) ++n;
    return n;
}

}  // namespace sm64ds::remote
