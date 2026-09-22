// Port stand-in for the ROM's LoadUnusedType13Objects (ov002 0x020fe3e4).
// Object type 13 is unused in the shipped game: the loader stashes the
// entry array's address in data_0209f338[0] and walks nothing. Upstream's
// TU (src/stage/LevelObjects.cpp) carries the real body for the ROM build;
// this C-named copy serves the host link (see the TU's own note).
extern int data_0209f338[];

struct LoadUnusedType13_Entry;
struct LoadUnusedType13_Tbl {
    unsigned char pad0;
    unsigned char count;
    unsigned char pad2[2];
    void *entries;
};

void _Z23LoadUnusedType13ObjectsRN11LVL_Overlay11ObjSubTableEij(
    struct LoadUnusedType13_Tbl *tbl, int areaID, unsigned param)
{
    (void)areaID;
    (void)param;
    data_0209f338[0] = (int)tbl->entries;
}
