# Online-startup constructor-table memory corruption

A failed full DS main-RAM reservation does not mean all writable pages in that
address range belong to the game. Windows can already own heap or mapped-file
allocations there. The constructor runner previously accepted any committed,
writable page at 0x02086b60 and wrote its callback table there. This could corrupt
another allocation during the menu-to-online-game handoff and later fault in a
Windows DLL, obscuring the original write.

The DS backend now records successful partial main-RAM reservations and exposes
io_owns_range. The constructor publisher requires ownership of the whole table
before checking protection and running the cartridge's literal-address loop.
A foreign or unavailable page selects the existing direct host-table fallback.
The ROM sources and constructor ordering are unchanged.

The controlled Windows regression launches a test process suspended, reserves a
foreign 64-KB allocation at 0x02080000 before loader/TLS initialization, places a
96-byte marker at 0x02086b60 and resumes startup. Preview.3 overwrites the marker;
the repaired binary preserves it and completes 120 gameplay frames. This tests
allocation ownership, not merely whether a memory access can succeed.

    python port/tools/coop_foreign_memory_test.py --exe GAME.exe --assets EXTRACTED_ROOT --output ISOLATED_OUTPUT

Add --baseline to demonstrate the previous unsafe behavior with preview.3.
Each output directory must be new. Tests use isolated saves/settings and silence
audio. Never run the regression with a personal save as its output path.

Seven isolated clients also completed the full animated-menu-to-online-game
handoff with readable names, palettes, the speed/coordinates HUD, and zero local
input-buffer frames under 90-ms delay/30-ms jitter. This local fixture verifies
seven-seat startup; it does not certify every internet connection or later level.
