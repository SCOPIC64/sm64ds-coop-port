//cpp
extern "C" void func_ov002_020db54c(void* s, int a, int b, int c);
extern "C" int port_actor_is_live(const void *o);

extern "C" int func_ov002_020da9d4(char* self){
    char* s = *(char**)(self + 0x358);
    int b = (s != 0);
    int* p;
    if (!b)
        return 0;
    /* PORT DIVERGENCE (memory safety): same dangling-held-actor guard as
       func_ov002_020daa74 -- the slot can outlive its actor across the
       cleanup phase. Drop it instead of faulting. */
    if (!port_actor_is_live(s)) {
        *(char**)(self + 0x358) = 0;
        return 1;
    }
    {
        int e = (*(unsigned short*)(s + 0xc) == 0xbf);
        if (e)
            func_ov002_020db54c(s, 0x10000, 0x10000, *(short*)(self + 0x8e));
    }
    p = (int*)(((int)*(char**)(self + 0x358) + 0xb0));
    *p &= ~0x4000;
    p = (int*)(((int)*(char**)(self + 0x358) + 0xb0));
    *p |= 0x2000;
    p = (int*)(((int)*(char**)(self + 0x358) + 0xb0));
    *p &= ~0x100;
    *(char**)(self + 0x358) = 0;
    return 1;
}
