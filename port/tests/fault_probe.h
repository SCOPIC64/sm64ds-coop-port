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

/* ---- crash.txt: the host crash screen -------------------------------------
   The stderr print above needs a live CRT and a captured stderr; a crash that
   kills the CRT (stack overflow, stdio corruption) or a run without redirection
   leaves nothing. This writes the same facts to crash.txt NEXT TO THE EXE with
   raw Win32 only -- static buffers, CreateFileA/WriteFile, hand-rolled hex, no
   stdio -- so the next silent death still names an address. The DS game ships
   its own crash screen (ShowCrashScreen); this is the host's.

   Installed twice: a FIRST-chance vectored handler on the codes that are fatal
   in this codebase (nothing here handles an AV), so the file is written even
   when the unhandled-filter never runs (exhausted stack, CRT death); and from
   the unhandled filter for everything that does reach it. CREATE_ALWAYS: the
   file describes the LAST crash. Offsets resolve against the /MAP file the
   build already writes (build/port/walk_window.map): section offset =
   printed offset - 0x1000 for .text symbols.

   STATUS_STACK_BUFFER_OVERRUN (0xC0000409 via __fastfail/int 29h) never raises
   a catchable exception on modern CPUs; if crash.txt stays absent across a
   repro, a /GS cookie or fastfail path is the remaining suspect. */
static void port_crash_hex(char *dst, unsigned v)
{
    static const char h[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 8; ++i)
        dst[i] = h[(v >> (28 - 4 * i)) & 0xf];
}

static void port_crash_write_file(EXCEPTION_POINTERS *ep)
{
    static char path[MAX_PATH + 16];
    static char buf[2048];
    static volatile LONG once;
    if (InterlockedExchange((volatile LONG *)&once, 1))
        return;                          /* first crash wins; no re-entry */
    {
        DWORD n = GetModuleFileNameA(0, path, MAX_PATH);
        while (n && path[n - 1] != 92 /* '\\' */)
            --n;
        lstrcpyA(path + n, "crash.txt");
    }
    {
    char *base = (char *)GetModuleHandleA(0);
    unsigned p = 0;
#define PORT_CRASH_STR(s) do { const char *q = (s); \
        while (*q && p < sizeof buf - 12) buf[p++] = *q++; } while (0)
#define PORT_CRASH_HEX(v) do { if (p < sizeof buf - 12) { \
        port_crash_hex(buf + p, (unsigned)(v)); p += 8; } } while (0)
    PORT_CRASH_STR("walk_window crash\r\ncode      ");
    PORT_CRASH_HEX(ep->ExceptionRecord->ExceptionCode);
    PORT_CRASH_STR("\r\naddress   ");
    PORT_CRASH_HEX((uintptr_t)ep->ExceptionRecord->ExceptionAddress);
    PORT_CRASH_STR("\r\nmodule    ");
    PORT_CRASH_HEX((uintptr_t)base);
    PORT_CRASH_STR("\r\noffset    +");
    PORT_CRASH_HEX((uintptr_t)ep->ExceptionRecord->ExceptionAddress
                   - (uintptr_t)base);
    if (ep->ExceptionRecord->NumberParameters > 1) {
        PORT_CRASH_STR("\r\naccess    ");
        PORT_CRASH_HEX(ep->ExceptionRecord->ExceptionInformation[0]);
        PORT_CRASH_STR(" at ");
        PORT_CRASH_HEX(ep->ExceptionRecord->ExceptionInformation[1]);
    }
    if (ep->ContextRecord) {
        CONTEXT *cx = ep->ContextRecord;
        unsigned *sp;
        int i, printed;
        PORT_CRASH_STR("\r\neip ");  PORT_CRASH_HEX(cx->Eip);
        PORT_CRASH_STR(" esp ");     PORT_CRASH_HEX(cx->Esp);
        PORT_CRASH_STR(" ebp ");     PORT_CRASH_HEX(cx->Ebp);
        PORT_CRASH_STR("\r\neax ");  PORT_CRASH_HEX(cx->Eax);
        PORT_CRASH_STR(" ebx ");     PORT_CRASH_HEX(cx->Ebx);
        PORT_CRASH_STR(" ecx ");     PORT_CRASH_HEX(cx->Ecx);
        PORT_CRASH_STR(" edx ");     PORT_CRASH_HEX(cx->Edx);
        PORT_CRASH_STR(" esi ");     PORT_CRASH_HEX(cx->Esi);
        PORT_CRASH_STR(" edi ");     PORT_CRASH_HEX(cx->Edi);
        /* module-relative return-address candidates off the faulting stack;
           IsBadReadPtr guards the walk so a torn ESP cannot re-fault */
        PORT_CRASH_STR("\r\nstack (+module words)");
        sp = (unsigned *)cx->Esp;
        printed = 0;
        for (i = 0; i < 256 && printed < 16; ++i) {
            unsigned v;
            if (IsBadReadPtr(sp + i, 4)) break;
            v = sp[i];
            if (v >= (unsigned)(uintptr_t)base &&
                v < (unsigned)(uintptr_t)base + 0x200000) {
                PORT_CRASH_STR("\r\n  +");
                PORT_CRASH_HEX(v - (unsigned)(uintptr_t)base);
                ++printed;
            }
        }
    }
    PORT_CRASH_STR("\r\nresolve: offset -> build/port/walk_window.map\r\n");
#undef PORT_CRASH_STR
#undef PORT_CRASH_HEX
    {
        HANDLE f = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD wr;
            WriteFile(f, buf, p, &wr, 0);
            FlushFileBuffers(f);
            CloseHandle(f);
        }
    }
    }
}

static LONG WINAPI port_crash_veh(EXCEPTION_POINTERS *ep)
{
    switch (ep->ExceptionRecord->ExceptionCode) {
    case 0xC0000005u:   /* access violation */
    case 0xC0000006u:   /* in-page error */
    case 0xC000001Du:   /* illegal instruction */
    case 0xC0000094u:   /* integer divide by zero */
    case 0xC0000096u:   /* privileged instruction */
    case 0xC00000FDu:   /* stack overflow: the UEF may never get a stack */
    case 0xC0000409u:   /* stack-buffer-overrun, when raised as an exception */
        port_crash_write_file(ep);
        break;
    }
    return EXCEPTION_CONTINUE_SEARCH;    /* normal handling still runs */
}

static LONG WINAPI port_fault_probe_with_file(EXCEPTION_POINTERS *ep)
{
    port_crash_write_file(ep);
    return port_fault_probe(ep);
}

/* Redirected stdio is fully buffered under the MSVC CRT, so a hard death
   loses every line since the last flush; the two setvbuf calls make captures
   loss-free. Harmless on a console. */
#define PORT_INSTALL_FAULT_PROBE() do { \
        setvbuf(stderr, NULL, _IONBF, 0); \
        setvbuf(stdout, NULL, _IONBF, 0); \
        AddVectoredExceptionHandler(1, port_crash_veh); \
        SetUnhandledExceptionFilter(port_fault_probe_with_file); \
    } while (0)

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
