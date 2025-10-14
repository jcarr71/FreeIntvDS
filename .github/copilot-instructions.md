Overview
- This repo is the FreeIntv libretro core (C). It builds into a libretro plugin (freeintv_libretro.*) and depends on libretro-common headers in `src/deps/libretro-common`.

Key areas for an agent to read first
- `README.md` — high-level constraints: requires `exec.bin` and `grom.bin` in the libretro system dir.
- `Makefile` and `Makefile.common` — primary build logic and platform flags. Target is `freeintv_libretro.(so|dll|dylib)`.
- `src/` — core implementation. Start with `src/libretro.c` (libretro glue), then `src/intv.c` (emulator loop/load/reset), and `src/cp1610.c` (CPU implementation).
- `jni/Android.mk` / `Application.mk` — how the Android build is wired (useful when adding Android support).

Architecture summary (big picture)
- This is a single-process, single-threaded emulation core. The libretro entry points (`retro_init`, `retro_run`, `retro_get_system_av_info`, etc.) live in `src/libretro.c` and call into emulator modules in `src/`.
- Major components:
  - CPU: `src/cp1610.c` (CP1610 instruction table and tick loop).
  - Core loop + ROM/BIOS loading: `src/intv.c` (LoadGame, loadExec, Run, exec())
  - Video: `src/stic.c` (STIC emulation, draw frame called from `exec()`)
  - Audio: `src/psg.c` and `src/ivoice.c`
  - IO/Controller/OSD: `src/controller.c`, `src/osd.c`
- Data flow: libretro input -> `retro_run()` -> input state translated into Intellivision controller state -> `Run()` invokes `exec()` which advances CPU cycles -> STIC renders frames and PSG/ivoice generate audio -> libretro callbacks `Video()` and `Audio()` sink output.

Build and developer workflows (discovered)
- Normal build (Unix / Windows MinGW): run `make` in the repo root. It uses `Makefile` + `Makefile.common`.
- Cross-platform flags: set `platform=` (e.g., `make platform=osx`, `make platform=libnx`) or set `DEBUG=1` for debug symbols.
- Android: `jni/Android.mk` references `Makefile.common`; build via the Android NDK build system (`ndk-build`/Gradle) using the `jni` directory.
- Quick checks and tests: there are no automated unit tests in the repo. Fast checks are: `make clean && make` and then load the resulting `freeintv_libretro.*` in a libretro frontend (RetroArch) with `exec.bin`/`grom.bin` available.

- Dual-screen variant: A `freeintvds` target was added. Run `make freeintvds` to build `freeintvds_libretro.*`.
- Implementation note: DS-specific code should be guarded by the `FREEINTV_DS` compile define (compile with `-DFREEINTV_DS`) and placed in `src/ds_stub.c` (or other files added via the `EXTRA_SOURCES` make variable). `Makefile.common` accepts `EXTRA_SOURCES` so DS-only files can be appended when building.

Project-specific conventions & patterns
- Libretro-first structure: public API is through `src/libretro.c`. Changes to module APIs must preserve libretro callback usage (Video, Audio, InputPoll, etc.).
- Single-file responsibilities: each hardware component is a single C file (cpu, stic, psg, ivoice); prefer edits within the owning file for local behavior.
- Serialization/versioning: save state uses a `SERIALIZED_VERSION` in `src/libretro.c`; when changing state layout update serialize/deserialize code and bump version.
- BIOS paths: `retro_init()` calls `Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath)` and then expects `exec.bin` and `grom.bin` in that directory — don't hardcode paths.

Integration points & external deps
- Depends on `libretro-common` under `src/deps/libretro-common` for portability helpers (file_path, compat). When adding features prefer these helpers.
- Build-time: compiler toolchain/flags are controlled by `Makefile`. Platform-specific blocks (Windows/MSVC, libnx, emscripten) are explicit there.
- Runtime: integrates with libretro frontend via the callbacks defined in `libretro.h` and `libretro-core-options.h`.

Quick editing rules for an AI code agent
- Preserve existing public API and ABI across `src/*.c` unless the task explicitly updates save-state or libretro interfaces.
- When adding new files: update `Makefile.common` `SOURCES_C` list or `Makefile` `SOURCES_C` entries (the repo collects sources there).
- Update `link.T` only if symbols/exports change (rare).
- If changing save-state, bump `SERIALIZED_VERSION` and update `retro_serialize()` / `retro_unserialize()` accordingly (see `src/libretro.c`).

Examples (short)
- To find where an input mapping is done: `src/libretro.c` -> `retro_run()` builds `joypad0[]`/`joypad1[]` and calls `setControllerInput()` in `src/controller.c`.
- To change CPU timing: edit `CP1610Tick()` in `src/cp1610.c` and consider downstream effects on `PSGTick()`/`ivoice_tk()` calls in `src/intv.c`.

What to ask the maintainers
- Preferred target platforms to test (Linux, Windows, Android, libnx?).
- Any planned API/serialization changes that the agent should avoid touching.

If anything above is unclear or you want the file to emphasize other areas (tests, CI, Android packaging), say which sections to expand.
