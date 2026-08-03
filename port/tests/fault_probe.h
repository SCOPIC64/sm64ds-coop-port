// Shared crash probe for the gate smokes: prints the module-relative fault
// address and a frame-pointer backtrace, resolvable against the /MAP file.
#ifndef PORT_FAULT_PROBE_H
#define PORT_FAULT_PROBE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static LONG WINAPI port_fault_probe(EXCEPTION_POINTERS *ep)
{
    char *base = (char *)GetModuleHandleA(0);
    fprintf(stderr, "FAULT code %08lx at +0x%08x accessing %08x\n",
            ep->ExceptionRecord->ExceptionCode,
            (unsigned)((char *)ep->ExceptionRecord->ExceptionAddress - base),
            (unsigned)(ep->ExceptionRecord->NumberParameters > 1
                       ? ep->ExceptionRecord->ExceptionInformation[1] : 0));
    void *frames[12];
    unsigned n = CaptureStackBackTrace(0, 12, frames, 0);
    for (unsigned i = 0; i < n; ++i)
        fprintf(stderr, "  frame %u: +0x%08x\n", i,
                (unsigned)((char *)frames[i] - base));
    /* raw return-address candidates off the FAULTING stack (the frames
       above are the handler's own); module-relative, /MAP-resolvable */
    if (ep->ContextRecord) {
        CONTEXT *cx = ep->ContextRecord;
        fprintf(stderr,
                "  regs eax=%08x ecx=%08x edx=%08x ebx=%08x esi=%08x "
                "edi=%08x ebp=%08x\n",
                (unsigned)cx->Eax, (unsigned)cx->Ecx, (unsigned)cx->Edx,
                (unsigned)cx->Ebx, (unsigned)cx->Esi, (unsigned)cx->Edi,
                (unsigned)cx->Ebp);
    }
    if (ep->ContextRecord) {
        unsigned *sp = (unsigned *)ep->ContextRecord->Esp;
        for (int i = 0; i < 96; ++i) {
            unsigned v;
            if (IsBadReadPtr(sp + i, 4)) break;
            v = sp[i];
            if (v >= (unsigned)(uintptr_t)base &&
                v < (unsigned)(uintptr_t)base + 0x200000)
                fprintf(stderr, "  stack[%02d] +0x%08x\n", i,
                        (unsigned)(v - (unsigned)(uintptr_t)base));
        }
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

#define PORT_INSTALL_FAULT_PROBE() SetUnhandledExceptionFilter(port_fault_probe)

/* Hang watchdog (PORT_WATCHDOG=<seconds>): a helper thread suspends the
   main thread after the deadline and prints its EIP plus a raw stack
   sample, module-relative, resolvable against the /MAP file. For loops
   the fault probe never sees. */
struct port_watchdog_args { HANDLE main; unsigned secs; };

static DWORD WINAPI port_watchdog_thread(LPVOID p)
{
    struct port_watchdog_args *a = (struct port_watchdog_args *)p;
    Sleep(a->secs * 1000);
    char *base = (char *)GetModuleHandleA(0);
    SuspendThread(a->main);
    CONTEXT ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (GetThreadContext(a->main, &ctx)) {
        fprintf(stderr, "WATCHDOG eip=+0x%08x esp=%08x\n",
                (unsigned)((char *)ctx.Eip - base), (unsigned)ctx.Esp);
        /* raw return-address candidates off the live stack */
        unsigned *sp = (unsigned *)ctx.Esp;
        for (int i = 0; i < 96; ++i) {
            unsigned v;
            if (IsBadReadPtr(sp + i, 4)) break;
            v = sp[i];
            if (v >= (unsigned)(uintptr_t)base &&
                v < (unsigned)(uintptr_t)base + 0x200000)
                fprintf(stderr, "  stack[%02d] +0x%08x\n", i,
                        (unsigned)(v - (unsigned)(uintptr_t)base));
        }
    }
    fflush(stderr);
    TerminateProcess(GetCurrentProcess(), 3);
    return 0;
}

/* Hardware write-watch (x86 debug registers): up to 4 dword slots.
   port_watch_words(addr, n) arms DR0..DR3 on the calling thread; the
   vectored handler prints the writer's module-relative EIP and keeps
   going. For finding who stomps a host global. */
static LONG WINAPI port_watch_handler(EXCEPTION_POINTERS *ep)
{
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
        return EXCEPTION_CONTINUE_SEARCH;
    char *base = (char *)GetModuleHandleA(0);
    static int shown;
    if (shown < 8) {
        ++shown;
        fprintf(stderr, "[watch] write near watched words, eip=+0x%08x\n",
                (unsigned)((char *)ep->ExceptionRecord->ExceptionAddress -
                           base));
        unsigned *sp = (unsigned *)ep->ContextRecord->Esp;
        int printed = 0;
        for (int i = 0; i < 64 && printed < 4; ++i) {
            unsigned v;
            if (IsBadReadPtr(sp + i, 4)) break;
            v = sp[i];
            if (v >= (unsigned)(uintptr_t)base &&
                v < (unsigned)(uintptr_t)base + 0x200000) {
                fprintf(stderr, "    caller? +0x%08x\n",
                        (unsigned)(v - (unsigned)(uintptr_t)base));
                ++printed;
            }
        }
        fflush(stderr);
    }
    ep->ContextRecord->EFlags |= 0x10000;   /* RF: resume past the hit */
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void port_watch_words(void *addr, int nwords)
{
    static int handler_in;
    if (!handler_in) {
        handler_in = 1;
        AddVectoredExceptionHandler(1, port_watch_handler);
    }
    CONTEXT ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    HANDLE th = GetCurrentThread();
    GetThreadContext(th, &ctx);
    DWORD *dr[4] = {&ctx.Dr0, &ctx.Dr1, &ctx.Dr2, &ctx.Dr3};
    if (nwords > 4) nwords = 4;
    for (int i = 0; i < nwords; ++i) {
        *dr[i] = (DWORD)(uintptr_t)((char *)addr + 4 * i);
        ctx.Dr7 |= (1u << (2 * i));                 /* local enable */
        ctx.Dr7 |= (0x1u << (16 + 4 * i));          /* break on WRITE */
        ctx.Dr7 |= (0x3u << (18 + 4 * i));          /* len = 4 bytes */
    }
    SetThreadContext(th, &ctx);
}

static void port_install_watchdog(void)
{
    const char *e = getenv("PORT_WATCHDOG");
    if (!e) return;
    static struct port_watchdog_args a;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                    GetCurrentProcess(), &a.main, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    a.secs = (unsigned)atoi(e);
    if (!a.secs) a.secs = 10;
    CreateThread(0, 0, port_watchdog_thread, &a, 0, 0);
}

#endif
