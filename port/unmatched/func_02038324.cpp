// HOST COPY of src/func_02038324.cpp -- identical flow plus a bounds
// guard on the collider-slot index. The registry has 24 slots and 0x18
// is the "empty" sentinel; anything else outside the range is a record
// a host probe filled incompletely (the ROM's manager wrapper writes
// result.clsn, direct DetectClsn calls do not). Trace once and skip
// instead of dereferencing a stranger's memory.
#include <stdio.h>

extern "C" {
int func_02037fc0(void *p);
int func_02037f54(void *p);
int func_020393cc(void *p);
int func_02038298(void *p);
int func_020393b4(void *p);
}
extern void *data_020a0c80[];

struct Obj {
    virtual void m0();
    virtual void m1();
    virtual void m2();
    virtual void m3();
    virtual void m4();
    virtual void m5();
    virtual void m6();
    virtual void m7();
    virtual void m8();
    virtual void m9(void *a, int v, int b, int c, int d);
};

extern "C" void func_02038324(void *arg, int b, int c, int d)
{
    int idx;
    Obj *obj;
    if (func_02037fc0(arg) == 0) return;
    idx = func_02037f54(arg);
    if (idx < 0 || idx >= 24) {
        static int warned;
        if (!warned) {
            warned = 1;
            fprintf(stderr, "  [clsn] surface record slot 0x%x out of "
                            "range (record %p) -- skipped\n", idx, arg);
        }
        return;
    }
    obj = (Obj *)data_020a0c80[idx];
    if (obj == 0) return;
    if (func_020393cc(obj) == 0) return;
    if (func_02038298(arg) == 0) return;
    obj->m9(arg, func_020393b4(data_020a0c80[idx]), b, c, d);
}
