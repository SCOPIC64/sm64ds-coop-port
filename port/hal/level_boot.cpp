// The real level boot, host side.
//
// Everything the game needs to load the castle grounds for real -- the level
// overlay, the collision file, the object tables -- instead of the harness's
// hand-staged KCL and invented spawn point. Nothing here is behaviour:
// Stage::LoadClsnAndObjects and its fifteen sub-loaders are the matched src
// files, and this is the seam they need.
//
// ---- the overlay mount ----------------------------------------------------
//
// Castle grounds is level 1, whose LVL_Overlay lives in ov009 at 0x02112bdc.
// A DS overlay is linked at a fixed base and loaded there unrelocated, so the
// ROM image already carries absolute pointers -- the object tables, the CLPS
// block, the path nodes. Mounting it on the host is therefore two steps:
// copy the whole image into one host array (ovdata.py --whole; per-symbol
// arrays break every walk that steps past the symbol dsd happened to name),
// then rewrite the words the delink table says point back inside it.
//
// port_ov009_at() turns a DS address into the host address of the same byte,
// which is how every constant below is spelled.
#include <cstdio>
#include <cstdlib>

extern "C" {
void port_ov009_patch(void);
void *port_ov009_at(unsigned ds);
extern unsigned char port_ov009_image[];
extern const unsigned port_ov009_ds_base, port_ov009_ds_end;
}

/* Castle grounds, level 1. The LVL_Overlay record and the two tables the
   Stage-A probes read straight out of it. */
enum {
    OV009_LVL_OVERLAY = 0x02112bdc,
};

/* LVL_Overlay, the fields the boot uses. */
struct PortLvlOverlay {
    unsigned char *clps;         /* 0x00 */
    unsigned char *objTable;     /* 0x04 */
    unsigned short bmdFileId;    /* 0x08 */
    unsigned short kclFileId;    /* 0x0a */
    unsigned short icgFileId;    /* 0x0c */
    unsigned short iclFileId;    /* 0x0e */
    unsigned char *subTables;    /* 0x10, stride 0xc */
    unsigned char subCount;      /* 0x14 */
    unsigned char flags;         /* 0x15 */
    unsigned char pad16[2];
    unsigned int unk18;          /* 0x18 */
};

extern "C" void *port_ov009_mount(void)
{
    static void *lvl;
    if (lvl)
        return lvl;
    port_ov009_patch();
    lvl = port_ov009_at(OV009_LVL_OVERLAY);
    if (!lvl) {
        std::fprintf(stderr, "FATAL: ov009 mount: 0x%08x outside the overlay "
                     "[0x%08x, 0x%08x)\n", (unsigned)OV009_LVL_OVERLAY,
                     port_ov009_ds_base, port_ov009_ds_end);
        std::abort();
    }
    return lvl;
}

/* Bring-up probe: the mount is a pointer rewrite over Nintendo bytes, so
   print what the game will actually read rather than trusting the rebase. */
extern "C" void port_ov009_probe(void)
{
    const PortLvlOverlay *o = (const PortLvlOverlay *)port_ov009_mount();
    std::printf("[ov009] image %p .. %p (DS 0x%08x .. 0x%08x)\n",
                (void *)port_ov009_image,
                (void *)(port_ov009_image +
                         (port_ov009_ds_end - port_ov009_ds_base)),
                port_ov009_ds_base, port_ov009_ds_end);
    std::printf("[ov009] LVL_Overlay: clps %p objTable %p bmd %u kcl %u "
                "subTables %p subCount %u flags %02x\n",
                (void *)o->clps, (void *)o->objTable, o->bmdFileId,
                o->kclFileId, (void *)o->subTables, o->subCount, o->flags);
    /* the main object table: u16 count, entry pointer, 8-byte records of
       {u8 kind, u8 count, pad, ObjTable* entries} */
    unsigned n = *(const unsigned short *)o->objTable;
    const unsigned char *e = *(const unsigned char *const *)(o->objTable + 4);
    std::printf("[ov009] objTable: %u kinds at %p\n", n, (const void *)e);
    for (unsigned i = 0; i < n; ++i, e += 8)
        std::printf("        kind 0x%02x (grp %d idx %2d) count %3u entries %p\n",
                    e[0], (e[0] >> 5) & 7, e[0] & 0x1f, e[1],
                    *(const void *const *)(e + 4));
    for (unsigned s = 0; s < o->subCount; ++s) {
        const unsigned char *t =
            *(const unsigned char *const *)(o->subTables + s * 0xc);
        if (!t)
            continue;
        unsigned m = *(const unsigned short *)t;
        const unsigned char *se = *(const unsigned char *const *)(t + 4);
        std::printf("[ov009] sub[%u] table %p: %u kinds\n", s,
                    (const void *)t, m);
        for (unsigned i = 0; i < m; ++i, se += 8)
            std::printf("        kind 0x%02x (grp %d idx %2d) count %3u "
                        "entries %p\n", se[0], (se[0] >> 5) & 7, se[0] & 0x1f,
                        se[1], *(const void *const *)(se + 4));
    }
}
