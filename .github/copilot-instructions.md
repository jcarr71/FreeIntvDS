# FreeIntvDS Copilot Instructions

## Overview
This repo contains the FreeIntvDS libretro core (C), a dual-screen Intellivision emulator core. It builds into a libretro plugin (`freeintvds_libretro.*`) and depends on libretro-common headers in `src/deps/libretro-common`. FreeIntvDS is distinct from the original FreeIntv core and features side-by-side dual-screen display: game on the left (352x224) and controller overlay on the right (256x224).

## Key areas for an agent to read first
- `README.md` — high-level constraints: requires `exec.bin` and `grom.bin` in the libretro system dir.
- `DUAL_SCREEN_DEVELOPMENT_SUMMARY.md` — current dual-screen implementation status and technical details.
- `Makefile` and `Makefile.common` — primary build logic and platform flags. Main target: `freeintvds_libretro.(so|dll|dylib)`.
- `src/libretro.c` — libretro glue and dual-screen implementation (608x224 combined resolution).
- `src/intv.c` — emulator loop/load/reset, and `src/cp1610.c` — CPU implementation.
- `jni/Android.mk` / `Application.mk` — Android build configuration.

## Architecture summary (big picture)
- Single-process, single-threaded emulation core with dual-screen libretro implementation.
- **Dual-screen layout**: Side-by-side rendering in single 608x224 texture buffer (352+256 width).
- **PNG overlay system**: Loads controller overlays from `overlays/[ROM_NAME].png` relative to ROM path.
- Major components:
  - CPU: `src/cp1610.c` (CP1610 instruction table and tick loop)
  - Core loop + ROM/BIOS loading: `src/intv.c` (LoadGame, loadExec, Run, exec())
  - Video: `src/stic.c` (STIC emulation, renders left screen) + dual-screen logic in `src/libretro.c`
  - Audio: `src/psg.c` and `src/ivoice.c`
  - IO/Controller/OSD: `src/controller.c`, `src/osd.c`
- **Data flow**: libretro input → `retro_run()` → input state translated → `Run()` invokes `exec()` → STIC renders game frame → dual-screen compositor combines game + overlay → single texture to libretro frontend.

## Build and developer workflows
 **Windows build workflow**:
 - Install MinGW-w64 and ensure its `bin` directory is in your PATH.
 - Use `mingw32-make` instead of `make` in PowerShell or VS Code terminal:
   ```powershell
   mingw32-make clean
   mingw32-make        # Builds freeintvds_libretro.dll (dual-screen version)
   ```
 - Alternative: `mingw32-make freeintv` for original single-screen core.
 - If you update PATH, restart VS Code to reload environment variables.

 **Cross-platform and advanced builds**:
 - Set `platform=` (e.g., `make platform=osx`, `make platform=libnx`) or `DEBUG=1` for debug symbols.
 - For Android, use the NDK build system in the `jni` directory.

 **Quick checks and tests**:
 - No automated unit tests. Fast checks: build and load the resulting core in a libretro frontend (RetroArch) with `exec.bin`/`grom.bin` available.
 - Test dual-screen: Load ROM and check for 608x224 display with game on left, overlay on right.
 - Test overlay: Place PNG files in `overlays/[ROM_NAME].png` relative to ROM directory.
- Implementation note: DS-specific code should be guarded by the `FREEINTV_DS` compile define (compile with `-DFREEINTV_DS`) and placed in `src/ds_stub.c` (or other files added via the `EXTRA_SOURCES` make variable). `Makefile.common` accepts `EXTRA_SOURCES` so DS-only files can be appended when building.

Project-specific conventions & patterns
- Libretro-first structure: public API is through `src/libretro.c`. Changes to module APIs must preserve libretro callback usage (Video, Audio, InputPoll, etc.).
- Single-file responsibilities: each hardware component is a single C file (cpu, stic, psg, ivoice); prefer edits within the owning file for local behavior.
- Serialization/versioning: save state uses a `SERIALIZED_VERSION` in `src/libretro.c`; when changing state layout update serialize/deserialize code and bump version.
- BIOS paths: `retro_init()` calls `Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath)` and then expects `exec.bin` and `grom.bin` in that directory — don't hardcode paths.

**Overlay conventions**:
- Each game can have a custom numeric pad overlay PNG in an `overlays` subfolder relative to the ROM directory.
- Overlay PNG filenames must match the ROM filename without extension (e.g., `overlays/Space Armada (USA).png`).
- **Current implementation**: PNG detection and file opening works; PNG decompression in progress.
- Overlays render in right screen (256x224) while game renders in left screen (352x224).
- **File structure**: ROM at `/path/to/game.rom` looks for overlay at `/path/to/overlays/game.png`.
- Handles RetroArch ZIP extraction to temporary directories with dynamic path resolution.

Integration points & external deps
- Depends on `libretro-common` under `src/deps/libretro-common` for portability helpers (file_path, compat). When adding features prefer these helpers.
- Build-time: compiler toolchain/flags are controlled by `Makefile`. Platform-specific blocks (Windows/MSVC, libnx, emscripten) are explicit there.
- Runtime: integrates with libretro frontend via the callbacks defined in `libretro.h` and `libretro-core-options.h`.

Quick editing rules for an AI code agent
- Preserve existing public API and ABI across `src/*.c` unless the task explicitly updates save-state or libretro interfaces.
- When adding new files: update `Makefile.common` `SOURCES_C` list or `Makefile` `SOURCES_C` entries (the repo collects sources there).
- Update `link.T` only if symbols/exports change (rare).
- If changing save-state, bump `SERIALIZED_VERSION` and update `retro_serialize()` / `retro_unserialize()` accordingly (see `src/libretro.c`).

**Examples (short)**
- To find where an input mapping is done: `src/libretro.c` -> `retro_run()` builds `joypad0[]`/`joypad1[]` and calls `setControllerInput()` in `src/controller.c`.
- To change CPU timing: edit `CP1610Tick()` in `src/cp1610.c` and consider downstream effects on `PSGTick()`/`ivoice_tk()` calls in `src/intv.c`.
- Dual-screen rendering: `test_function()` in `src/libretro.c` combines STIC output (left) with overlay (right) into single 608x224 buffer.
- PNG overlay loading: `load_overlay_for_rom()` in `src/libretro.c` handles path resolution and file detection.

## Current development status
- **Dual-screen display**: ✅ Working (608x224 combined resolution)
- **PNG file detection**: ✅ Working (finds overlays relative to ROM path)
- **PNG loading**: ✅ Working (stb_image loads PNG data with correct dimensions)
- **Build system**: ✅ Working (`mingw32-make` produces `freeintvds_libretro.dll`)
- **Core identification**: ✅ Working (identifies as "FreeIntvDS" when `FREEINTV_DS` defined)
- **PNG display rendering**: ❌ **CURRENT ISSUE** - PNG loads correctly but displays as "color static"

## Active development issue (October 14, 2025)
**Problem**: PNG overlay files load successfully (correct dimensions, pixel count) but display as "color static" instead of coherent images.

**Confirmed working**:
- File detection: `overlays/[ROM_NAME].png` files found correctly
- PNG loading: stb_image loads 256x224 PNG with 57,344 pixels as expected
- Display pipeline: Corner color tests confirm 608x224 dual-screen rendering works
- Build chain: Clean compilation with MinGW-w64

**Investigation needed**:
- Pixel processing loop: RGB→ARGB conversion may have bugs
- Pixel data extraction: Verify PNG RGB values are read correctly
- Debug statistics: Variable scope issues prevent detailed pixel processing logs

**Next session priorities**:
1. Create controlled test PNG with known content (solid colors, simple patterns)
2. Debug pixel data extraction from PNG before ARGB conversion
3. Fix variable scope issues to restore pixel processing statistics
4. Verify ARGB conversion logic in pixel processing loop

**Test setup ready**:
- Working core: `freeintvds_libretro.dll`
- Test ROM: `open-content/4-Tris/4-tris.rom`
- Overlay directory: `open-content/4-Tris/overlays/` (created, ready for test PNG)
- Debug logging: Console output and `freeintvds_debug.txt` functional

## What to ask the maintainers
- Preferred target platforms to test (Linux, Windows, Android, libnx?).
- Any planned API/serialization changes that the agent should avoid touching.

If anything above is unclear or you want the file to emphasize other areas (tests, CI, Android packaging), say which sections to expand.
