# Native port progress

The Windows preview remains the only playable game package. This work ports
real runtime code but does not yet produce Linux/SteamOS games, a macOS app,
an Android APK or an iOS IPA.

## Completed here

- `ntr/rt.cpp` can suspend and resume a game stack on non-Windows hosts using
  compiled Minicoro context switching, preserving the existing single-thread
  scheduling model. VBlank keeps the game's nested call stack intact. Windows
  retains its existing fiber backend. Stopping abandons a suspended stack, like
  Windows `DeleteFiber`; host resources must live outside that stack.
- `port/native` builds the actual DS graphics/I/O/runtime library with Clang
  for 32-bit Linux. ELF explicitly sorts the existing DS state section family,
  so the IRQ callback banks and snapshot bounds retain their required layout.
- A generated host-only type header uses the compiler's own `size_t` and
  leaves ROM-verified source and shared headers unchanged. The native build
  compiles the real Player bridge and wait-state method with layout guards.
- The runtime check runs the actual `rt_run` implementation against fixed DS
  I/O mappings and the hardware IRQ model, checking nested VBlank, 263 HBlank
  edges per frame, natural completion, bounded stop, hook stop and restart.
- Separate context checks execute on Linux, Windows and macOS and compile the
  actual backend against Android ARM32/ARM64 and iOS ARM64 SDKs. These checks
  do not establish mobile gameplay or on-device touchscreen behavior.

## Building the native Linux core

Use Clang and a 32-bit C++ runtime, such as Ubuntu's `clang` and `g++-multilib`:

```sh
cmake -S port/native -B build/native-core -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-m32
cmake --build build/native-core --parallel 2
ctest --test-dir build/native-core --output-on-failure
```

The output is a native static hardware library and test executable, not a
game launcher. No ROM or extracted cartridge data is needed for these checks.

## Remaining work for playable ports

The full game's native window, controller, audio, networking, child-background
renderer, asset import and storage adapters are still missing. The existing
32-bit MSVC method/thunk and linker-alias bridges also need native calling-
convention verification and replacement; compiling an object does not prove
that its virtual calls or complete game link work.

The current Player storage is 1,896 bytes and its state entry is 24 bytes on
32-bit Linux. ARM64 probes produce 2,384-byte Players and 48-byte state entries,
while native pointers are eight bytes. The game still allocates and reads DS
offsets and stores pointers in four-byte words. ARM64 macOS, Android and iOS
therefore require a consistent guest/host pointer and call representation;
turning off the layout guards would produce memory corruption.

SteamOS shares the Linux engine. Its separate controller/fullscreen profile
remains `HANDHELD=ON`, following CoopDX's build-profile distinction. Android
and iOS additionally need visible touch controls wired to game/menu events,
safe-area and lifecycle handling, platform storage/import, and device tests.
Apple distribution also needs the user's signing configuration.
