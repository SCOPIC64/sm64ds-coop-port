// Use the real host storage; ELF joins the historical raw name in this TU.
#include "../hal/heap_globals.cpp"

extern "C" char data_020a4d38[0x20]
    __attribute__((alias("_ZN6Memory16rootHeapIteratorE")));
extern "C" int data_020a4d34
    __attribute__((alias("_ZN6Memory25isRootHeapIterInitializedE")));
