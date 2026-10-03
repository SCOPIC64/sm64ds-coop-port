#include <windows.h>
#include <cstdio>

extern "C" int func_02057020(void);
extern "C" void func_02057078(int);
extern "C" void port_os_lock_words_seed(void);
extern "C" void port_backup_transfer_end(int) {}

int main()
{
    void *page = VirtualAlloc((void *)0x027f0000, 0x10000,
                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (page != (void *)0x027f0000) return 2;
    const int filesystem = func_02057020();
    port_os_lock_words_seed();
    if (filesystem != 0x40 || func_02057020() != 0x41) return 3;
    for (int expected = 0x42; expected < 0x70; ++expected)
        if (func_02057020() != expected) return 4;
    if (func_02057020() != -3) return 5;
    for (int n = 0; n < 1000; ++n) {
        func_02057078(0x41);
        if (func_02057020() != 0x41) return 6;
        if (func_02057020() != -3) return 7;
    }
    VirtualFree(page, 0, MEM_RELEASE);
    std::puts("lockid_probe: PASS (early allocation, repeated init, exhaustion, 1000 reuses)");
    return 0;
}
