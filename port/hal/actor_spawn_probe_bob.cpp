// A DIRECT ACTOR SPAWN, for bringing a class up before its level exists.
//
// Gate 32 hosts Bob-omb Battlefield's cast, and the level boot is still the
// castle grounds' (hal/level_boot.cpp mounts ov009 by hand). So there is no
// object table that names a Goomba, and the way to see one run is to ask the
// ROM's own spawn entry point for one where the Player is standing.
//
// Nothing here is a new mechanism. Actor::Spawn is the matched veneer the
// level's LoadStandardObjects calls for every object it reads -- it seats the
// spawn context (position, rotation, area, death-table id) through
// func_02010e78 and then calls ActorDerived::Spawn under the scene root. This
// file passes the same six arguments from an environment variable instead of
// from a level record, so what runs afterwards is exactly what a level record
// would have produced.
//
//   SM64DS_BOB_SPAWN=id[:param][@dx,dy,dz][,id[:param]...]
//        one spawn per comma-separated term, `id` decimal or 0x-hex.
//        :param is the object record's param1 word, which is what selects a
//        Goomba's size or a Bob-omb's variant; the offset is in world units
//        from the Player and defaults to 200 units in front of him.
//   SM64DS_BOB_SPAWN_FRAME=N   the frame to do it on (default 60, so the
//        Player has landed and the camera has settled first)
//   SM64DS_BOB_SPAWN_AREA=N    the area id (default -1, "not area-bound",
//        which is what the level's own main table gives its objects)
//
// THIS IS THE GATE-32 COPY and it is scoped to this file on purpose: the
// level-boot stream (port-beta-lvl) is adding a spawn hook of its own at the
// same time. Whichever lands second should collapse the two; the env names
// here are unique so they cannot silently fight.
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

struct PortSpawnVec { int x, y, z; };
struct PortSpawnRot { short x, y, z; };

void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(unsigned id, unsigned param1,
                                                   const PortSpawnVec *pos,
                                                   const PortSpawnRot *rot,
                                                   signed char areaID,
                                                   short deathTableID);
extern short data_ov002_0211118c;      /* the per-level spawn counter */
const char *port_actor_class_name(unsigned id);

}  /* extern "C" */

/* One parsed term. Sixteen is more than any session has wanted. */
struct PortBobSpawn {
    unsigned id, param;
    int dx, dy, dz;
};
static PortBobSpawn g_terms[16];
static int g_count = -1;

static void port_bob_spawn_parse(void)
{
    const char *e = std::getenv("SM64DS_BOB_SPAWN");
    g_count = 0;
    if (!e)
        return;
    while (*e && g_count < 16) {
        char *end;
        PortBobSpawn t;
        t.param = 0;
        t.dx = 0;
        t.dy = 0;
        t.dz = 200;
        t.id = (unsigned)std::strtoul(e, &end, 0);
        if (end == e)
            break;
        e = end;
        if (*e == ':') {
            t.param = (unsigned)std::strtoul(e + 1, &end, 0);
            e = end;
        }
        if (*e == '@') {
            t.dx = (int)std::strtol(e + 1, &end, 0);
            e = (*end == ',') ? end + 1 : end;
            t.dy = (int)std::strtol(e, &end, 0);
            e = (*end == ',') ? end + 1 : end;
            t.dz = (int)std::strtol(e, &end, 0);
            e = end;
        }
        g_terms[g_count++] = t;
        if (*e == ',')
            ++e;
        else
            break;
    }
}

/* Called once per frame from the window's tick, before port_actor_tick. The
   spawns land on the PENDING list and the very next tick runs their
   InitResources, which is the same order the boot's own spawns take. */
extern "C" void port_bob_spawn_probe(void *player, int frame)
{
    static int fired;
    if (g_count < 0)
        port_bob_spawn_parse();
    if (!g_count || fired || !player)
        return;
    {
        const char *fe = std::getenv("SM64DS_BOB_SPAWN_FRAME");
        if (frame < (fe ? std::atoi(fe) : 60))
            return;
    }
    fired = 1;
    {
        const char *ae = std::getenv("SM64DS_BOB_SPAWN_AREA");
        const signed char area = (signed char)(ae ? std::atoi(ae) : -1);
        const char *c = (const char *)player;
        for (int i = 0; i < g_count; ++i) {
            PortSpawnVec pos;
            PortSpawnRot rot;
            void *a;
            pos.x = *(const int *)(c + 0x5c) + (g_terms[i].dx << 12);
            pos.y = *(const int *)(c + 0x60) + (g_terms[i].dy << 12);
            pos.z = *(const int *)(c + 0x64) + (g_terms[i].dz << 12);
            rot.x = 0;
            rot.y = 0;
            rot.z = 0;
            a = _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
                g_terms[i].id, g_terms[i].param, &pos, &rot, area,
                data_ov002_0211118c++);
            std::printf("[bobspawn] id %u (%s) param %u at (%d,%d,%d) -> %p\n",
                        g_terms[i].id, port_actor_class_name(g_terms[i].id),
                        g_terms[i].param, pos.x >> 12, pos.y >> 12,
                        pos.z >> 12, a);
        }
    }
}

/* What the spawned actors are doing, read back off the behaviour list. One
   line per live instance of every id the probe asked for: position, the
   Enemy-family state word and the flags Actor::BeforeBehavior wrote, which is
   what says whether it is being culled rather than misbehaving. */
extern "C" {
extern int data_020a4b78[8];
}

extern "C" void port_bob_spawn_report(void)
{
    if (g_count <= 0)
        return;
    for (int i = 0; i < g_count; ++i) {
        int n = 0;
        for (int *node = (int *)(size_t)data_020a4b78[0]; node && n < 4096;
             node = (int *)(size_t)node[1]) {
            const char *o = (const char *)(size_t)node[2];
            if (!o || *(const unsigned short *)(o + 0xc) != g_terms[i].id)
                continue;
            ++n;
            std::printf("[bobspawn] %-16s at (%6d,%6d,%6d) yaw %04x "
                        "flags %08x area %d\n",
                        port_actor_class_name(g_terms[i].id),
                        *(const int *)(o + 0x5c) >> 12,
                        *(const int *)(o + 0x60) >> 12,
                        *(const int *)(o + 0x64) >> 12,
                        (unsigned short)*(const short *)(o + 0x8e),
                        *(const unsigned *)(o + 0xb0),
                        *(const signed char *)(o + 0xcc));
        }
        if (!n)
            std::printf("[bobspawn] %s (id %u): no live instance\n",
                        port_actor_class_name(g_terms[i].id), g_terms[i].id);
    }
}
