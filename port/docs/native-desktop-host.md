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

Install SDL2 development headers (for example `libsdl2-dev` on Ubuntu, or `sdl2`
through Homebrew on macOS), then:

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
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS=-m32 \
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

Primary API references: [SDL window](https://wiki.libsdl.org/SDL2/SDL_CreateWindow),
[streaming textures](https://wiki.libsdl.org/SDL2/SDL_CreateTexture), and
[audio devices and queuing](https://wiki.libsdl.org/SDL2/SDL_OpenAudioDevice).
