# Native desktop host adapter

`port/desktop` provides real SDL window/renderer, input/controller and PCM audio
IO for Linux/SteamOS and macOS. It accepts the renderer's ARGB framebuffer and
preserves its stride. Rendering uses a streaming texture without a vsync wait;
the existing game frame scheduler continues to own timing.

The adapter reports physical drawable dimensions, including high DPI. The game
must configure its camera/framebuffer for that aspect to show outside-view in
fullscreen. Until the engine has supplied a matching frame, the adapter fits
the old frame without distortion. It never solves widescreen by stretching.

Keyboard and mouse-button codes and controller masks follow the existing PC
binding representation so the common three-slot binding layer can consume
them. Focus loss clears held keys and controller input. Repeat events cannot
reactivate a key held in another window; other-window events are ignored.
Controller removal clears the state, and reconnects select an available device.
Chat/name editors can request bounded UTF-8 text input. Window/renderer/input
calls and destruction belong to the main UI thread, as required for macOS.

Audio accepts queued interleaved signed-16 stereo PCM at the requested rate.
SDL handles any physical device conversion. The application queue is capped at
80 ms and rejects oversized/backlogged writes; it cannot accumulate seconds of
audio latency. Audio remains optional and does not block input or rendering.

## Build and check the adapter

The default build fetches and verifies SDL2 Classic 2.32.10 from the official
source archive and links it statically. This keeps SDL's implementation the
same across desktop targets. The initial Homebrew SDL2 compatibility layer
crashed inside its SDL3 text-event conversion during the macOS test; the host
must not silently depend on that unverified shim. The archive SHA256 is pinned
in CMake, and SDL's license must accompany eventual native game packages.

Install CMake, a C/C++ compiler and native graphics/audio development headers,
then:

```sh
cmake -S port/desktop -B build/native-desktop -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native-desktop --parallel 2
ctest --test-dir build/native-desktop --output-on-failure
```

The tests use dummy video/audio drivers and a virtual controller. They verify
actual SDL pixel readback before presentation, padded stride, resize/aspect,
focus/key/modifier behavior, UTF-8, controller/hotplug, fullscreen lifecycle,
bounded audio queuing, thread ownership and reopening. They do not establish
physical Steam Deck controller behavior, a visible Cocoa/Wayland game window,
or full game boot.

Linux32 can also link this host to the actual DS runtime/renderer:

```sh
cmake -S port/native -B build/native-desktop32 \
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_C_FLAGS=-m32 \
  -DNATIVE_DESKTOP_HOST_CHECKS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native-desktop32 --parallel 2
ctest --test-dir build/native-desktop32 --output-on-failure
```

Use 32-bit SDL development libraries and compiler runtime for this build. The
integrated fixture runs the actual DS frame coroutine, IRQ scanlines, palette
memory and PPU scanout through the SDL framebuffer and audio queue. It needs no
ROM and is not a playable native game. Complete native boot, ABI/virtual-call
bridges, menu/lobby/import/storage integration and gameplay/multiplayer tests
are still required. Modern macOS's game object/pointer representation remains
a separate blocker; compiling this pointer-independent host does not solve it.

Developers may choose `-DDESKTOP_BUNDLED_SDL2=OFF` to exercise a system SDL2
implementation separately. Passing that configuration's tests is required
before distributing it; the default native packages will use the pinned SDK.

Primary API references: [SDL window](https://wiki.libsdl.org/SDL2/SDL_CreateWindow),
[streaming textures](https://wiki.libsdl.org/SDL2/SDL_CreateTexture), and
[audio devices and queuing](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice).

The desktop storage adapter accepts an existing save/profile directory and
uses directory-relative POSIX file operations. It rejects path traversal,
symlinks on reads, non-regular files and oversized entries. Writes use private
temporary files, flush the data and atomically replace the destination; readers
observe a complete old or new save. The test exercises concurrent replacement,
file permissions, bounded reads and symlink/FIFO rejection on Linux and macOS.
Choosing the user data location and integrating existing game saves/preferences
remain separate work. No ROM or private server configuration is needed for this
fixture, and its success is not a playable native release.
