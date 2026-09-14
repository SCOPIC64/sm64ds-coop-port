# CRASH1 (respawn) heartbeat

- 20:00 EDT 2026-09-14 - spawned (respawn #2; first CRASH1 died at 19:42 with empty out/).
  ROOT C:/tmp/l7e on port/l7-crash1 @ 3b72845d4, clean apart from untracked port/build-port-k0.cmd.
  Identity Tango / 248217834+tangosdev@users.noreply.github.com. Starting: verify binary+map,
  resolve +0x272156, then static read of walk_window.cpp main.
- 20:12 EDT - STEP 1 DONE. Fault instruction found statically, no build needed.
  RVA 0x272156 = _main+0xdb6 (map: _main 0001:002703a0 = VA 006713a0, walk_window.cpp.obj).
  Disasm: 0067214a push edi / 0067214b push [0xa66f48 = g_mc] / 00672151 call 0x419070
  = _port_stage_a_boot (level_boot.cpp.obj) / 00672156 movzx eax, word ptr [eax+8] <-- FAULT.
  Source: port/tests/walk_window.cpp:8334-8335
    void *lvl = port_stage_a_boot(g_mc, boot_spawns);
    level_bmd = *(unsigned short *)((char *)lvl + 8);
  NULL = the return value of port_stage_a_boot. Now: step 2, why it returns 0.
- 20:30 EDT - STEP 2 narrowing. port_stage_a_boot returns g_boot_result, filled only if
  st_init (stage_bridges.cpp:350, _ZTV5Stage slot 0) runs. LINK21's FULL run.log survived at
  tmp/int4cap/A_level1_300_crosscheck/run.log: NO output from inside the boot at all, and
  regs edx=017a4a64 (= _data_020a4b88) / ecx=3003a6d8 (= g_stage+0x28) say func_020433b8
  ran to its LAST call func_0203b244(data_020a4b88, self+0x28) without taking an early return.
  func_020433b8 does NOT dispatch slot 0 itself: it delegates to func_0204335c =
  fBase_c::Process(self, data_02099ebc, data_02099ec4, data_02099e94), three mwcc
  pointer-to-member PAIRS. ALL THREE ARE ZERO IN THE SHIPPED IMAGE (.dsstate is fully
  initialized data; dumped at VA 017ab388/398/3a8 = 00000000 x4 each).
