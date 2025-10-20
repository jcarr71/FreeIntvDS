# FreeIntvDS Copilot Instructions# FreeIntvDS Copilot Instructions# FreeIntvDS Copilot Instructions



## Project Overview

FreeIntvDS is a libretro core for the Intellivision console with **dual-screen support**. It emulates Intellivision hardware (CP1610 CPU, STIC video, PSG audio, Intellivoice) and renders game + custom controller overlays. Designed for dual-screen handhelds like the Ayaneo Pocket DS.

## Project Overview## Overview

**Current architecture**: Vertical display layout — game rendered 704x448 (2x upscaled, top) + layered overlays 704x620 (bottom) = `704x1068` total texture. See `STATUS.md` for latest design details.

FreeIntvDS is a libretro core for the Intellivision console with **dual-screen support**. It emulates Intellivision hardware (CP1610 CPU, STIC video, PSG audio, Intellivoice) and renders game + custom controller overlays. Designed for dual-screen handhelds like the Ayaneo Pocket DS.This repo contains the FreeIntvDS libretro core (C), a dual-screen Intellivision emulator core. It builds into a libretro plugin (`freeintvds_libretro.*`) and depends on libretro-common headers in `src/deps/libretro-common`. FreeIntvDS is distinct from the original FreeIntv core and features side-by-side dual-screen display: game on the left (352x224) and controller overlay on the right (256x224).

## Essential Reading (Start Here)

1. **`README.md`** — BIOS requirements (`exec.bin`, `grom.bin` in RetroArch system dir), overlay setup paths, controller mappings

2. **`STATUS.md`** — Current feature checklist, display layout specifics, working features vs. pending tasks

3. **`Makefile.common`** — `SOURCES_C` list (authoritative source inventory), build flags**Current architecture**: Vertical display layout — game rendered 704x448 (2x upscaled, top) + layered overlays 704x620 (bottom) = `704x1068` total texture. See `STATUS.md` for latest design details.## Key areas for an agent to read first

4. **`src/libretro.c`** (skim lines 1-100, 812, 1124, 1441) — Entry points, dual-screen rendering, PNG loading

5. **`src/intv.c`** — Emulator core loop (`exec()`) and ROM/BIOS loading- `README.md` — high-level constraints: requires `exec.bin` and `grom.bin` in the libretro system dir.

6. **`src/cp1610.c`** — CPU instruction table and `CP1610Tick()` execution

## Essential Reading (Start Here)- `DUAL_SCREEN_DEVELOPMENT_SUMMARY.md` — current dual-screen implementation status and technical details.

## Architecture: Data Flow & Components

1. **`README.md`** — BIOS requirements (`exec.bin`, `grom.bin` in RetroArch system dir), overlay setup paths, controller mappings- `Makefile` and `Makefile.common` — primary build logic and platform flags. Main target: `freeintvds_libretro.(so|dll|dylib)`.

### Single-Threaded Emulation Loop

```2. **`STATUS.md`** — Current feature checklist, display layout specifics, working features vs. pending tasks- `src/libretro.c` — libretro glue and dual-screen implementation (608x224 combined resolution).

retro_run() [libretro.c]

  ↓ [Poll input via InputPoll/InputState, build joypad0/joypad1 arrays]3. **`Makefile.common`** — `SOURCES_C` list (authoritative source inventory), build flags- `src/intv.c` — emulator loop/load/reset, and `src/cp1610.c` — CPU implementation.

  ↓ setControllerInput() [controller.c: map joypad to hardware registers]

  ↓ Run() [intv.c → calls exec()]4. **`src/libretro.c`** (skim lines 1-100, 812, 1124, 1441) — Entry points, dual-screen rendering, PNG loading- `jni/Android.mk` / `Application.mk` — Android build configuration.

  ↓ exec() loop [intv.c]

    ↓ CP1610Tick() [cp1610.c: decode & execute one instruction]5. **`src/intv.c`** — Emulator core loop (`exec()`) and ROM/BIOS loading

    ↓ PSGTick() [psg.c: audio sample generation]

    ↓ ivoice_tk() [ivoice.c: voice synthesis]6. **`src/cp1610.c`** — CPU instruction table and `CP1610Tick()` execution## Architecture summary (big picture)

    ↓ STIC.updateDisplay [stic.c: render sprites/backgrounds → 352x224 BGR buffer]

  ↓ render_dual_screen() [libretro.c: upscale game 2x + composite overlays → 704x1068]- Single-process, single-threaded emulation core with dual-screen libretro implementation.

  ↓ video_cb() sends 704x1068 ARGB texture to RetroArch

```## Architecture: Data Flow & Components- **Dual-screen layout**: Side-by-side rendering in single 608x224 texture buffer (352+256 width).



### Hardware Emulation Components (One Module Each)- **PNG overlay system**: Loads controller overlays from `overlays/[ROM_NAME].png` relative to ROM path.

| Module | File | Key Functions |

|--------|------|---|### Single-Threaded Emulation Loop- Major components:

| **CPU** | `src/cp1610.c` | `CP1610Tick()` instruction dispatcher, `CP1610Serialize()` |

| **Video/Sprites** | `src/stic.c` | STIC registers, `updateDisplay()` → 352x224 BGR frame |```  - CPU: `src/cp1610.c` (CP1610 instruction table and tick loop)

| **Audio/PSG** | `src/psg.c` | PSG synthesis, `PSGTick()`, `PSGSerialize()` |

| **Voice** | `src/ivoice.c` | Intellivoice decoding, `ivoice_tk()`, `ivoiceSerialize()` |retro_run() [libretro.c]  - Core loop + ROM/BIOS loading: `src/intv.c` (LoadGame, loadExec, Run, exec())

| **Input** | `src/controller.c` | Keypad/disc state, `setControllerInput()` |

| **Memory** | `src/memory.c` | 16KB RAM, cartridge ROM, BIOS (exec/grom) banking |  ↓ [Poll input via InputPoll/InputState, build joypad0/joypad1 arrays]  - Video: `src/stic.c` (STIC emulation, renders left screen) + dual-screen logic in `src/libretro.c`

| **Cartridge** | `src/cart.c` | ROM file loading, `LoadCart()` |

| **Core Init/Loop** | `src/intv.c` | `LoadGame()`, `loadExec()`, `Run()`, `exec()` |  ↓ setControllerInput() [controller.c: map joypad to hardware registers]  - Audio: `src/psg.c` and `src/ivoice.c`



### Dual-Screen Rendering (704x1068 Vertical)  ↓ Run() [intv.c → calls exec()]  - IO/Controller/OSD: `src/controller.c`, `src/osd.c`

1. STIC outputs 352x224 BGR game frame

2. 2x pixel doubling → 704x448 (placed at top of output buffer)  ↓ exec() loop [intv.c]- **Data flow**: libretro input → `retro_run()` → input state translated → `Run()` invokes `exec()` → STIC renders game frame → dual-screen compositor combines game + overlay → single texture to libretro frontend.

3. Load controller_base.png from `<system>/freeintvds-overlays/` (446x620 pixels, PNG with alpha, right-aligned)

4. Load `[ROM_NAME].png` game-specific overlay (370x600 pixels, centered, fallback to default.png)    ↓ CP1610Tick() [cp1610.c: decode & execute one instruction]

5. Composite into 704x1068 ARGB buffer:

   - Black background (left 258 pixels of overlay area)    ↓ PSGTick() [psg.c: audio sample generation]## Build and developer workflows

   - Game overlay (back layer, centered)

   - Controller base (front layer, with alpha blending)    ↓ ivoice_tk() [ivoice.c: voice synthesis] **Windows build workflow**:



**Overlay path resolution**:    ↓ STIC.updateDisplay [stic.c: render sprites/backgrounds → 352x224 BGR buffer] - Install MinGW-w64 and ensure its `bin` directory is in your PATH.

- RetroArch system directory obtained in `retro_init()` via `RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY`

- Looks for: `<system>/freeintvds-overlays/<ROM_NAME>.png` → fallback `freeintvds-overlays/default.png`  ↓ render_dual_screen() [libretro.c: upscale game 2x + composite overlays → 704x1068] - Use `mingw32-make` instead of `make` in PowerShell or VS Code terminal:

- Handles ZIP extraction by parsing original ROM filename from path

  ↓ video_cb() sends 704x1068 ARGB texture to RetroArch   ```powershell

## Developer Workflows

```   mingw32-make clean

### Windows Build (MinGW-w64 Recommended)

```powershell   mingw32-make        # Builds freeintvds_libretro.dll (dual-screen version)

# First time: Install MinGW-w64, add bin/ to PATH, restart VS Code

### Hardware Emulation Components (One Module Each)   ```

mingw32-make clean              # Clean object files

mingw32-make                    # Build → freeintvds_libretro.dll (dual-screen)| Module | File | Key Functions | - Alternative: `mingw32-make freeintv` for original single-screen core.

mingw32-make DEBUG=1            # Build with debug symbols

mingw32-make freeintv           # Build original single-screen variant|--------|------|---| - If you update PATH, restart VS Code to reload environment variables.



# To build with extra sources| **CPU** | `src/cp1610.c` | `CP1610Tick()` instruction dispatcher, `CP1610Serialize()` |

mingw32-make EXTRA_SOURCES=src/my_feature.c

```| **Video/Sprites** | `src/stic.c` | STIC registers, `updateDisplay()` → 352x224 BGR frame | **Cross-platform and advanced builds**:



### Cross-Platform Builds| **Audio/PSG** | `src/psg.c` | PSG synthesis, `PSGTick()`, `PSGSerialize()` | - Set `platform=` (e.g., `make platform=osx`, `make platform=libnx`) or `DEBUG=1` for debug symbols.

```bash

make platform=osx               # macOS| **Voice** | `src/ivoice.c` | Intellivoice decoding, `ivoice_tk()`, `ivoiceSerialize()` | - For Android, use the NDK build system in the `jni` directory.

make platform=libnx             # Nintendo Switch

make platform=unix              # Linux generic| **Input** | `src/controller.c` | Keypad/disc state, `setControllerInput()` |

make platform=android           # Android (uses jni/Android.mk instead)

```| **Memory** | `src/memory.c` | 16KB RAM, cartridge ROM, BIOS (exec/grom) banking | **Quick checks and tests**:



### Local Testing (Manual)| **Cartridge** | `src/cart.c` | ROM file loading, `LoadCart()` | - No automated unit tests. Fast checks: build and load the resulting core in a libretro frontend (RetroArch) with `exec.bin`/`grom.bin` available.

No automated tests — validation is manual:

1. Copy compiled `.dll` / `.so` to RetroArch cores directory| **Core Init/Loop** | `src/intv.c` | `LoadGame()`, `loadExec()`, `Run()`, `exec()` | - Test dual-screen: Load ROM and check for 608x224 display with game on left, overlay on right.

2. Ensure `exec.bin` and `grom.bin` in RetroArch system directory

3. Load ROM in RetroArch - Test overlay: Place PNG files in `overlays/[ROM_NAME].png` relative to ROM directory.

4. Verify 704x1068 display with game + overlays rendering correctly

### Dual-Screen Rendering (704x1068 Vertical)- Implementation note: DS-specific code should be guarded by the `FREEINTV_DS` compile define (compile with `-DFREEINTV_DS`) and placed in `src/ds_stub.c` (or other files added via the `EXTRA_SOURCES` make variable). `Makefile.common` accepts `EXTRA_SOURCES` so DS-only files can be appended when building.

### Adding New Source Files

Update `Makefile.common` `SOURCES_C` list, or pass at build time:```c

```bash

mingw32-make EXTRA_SOURCES="src/new_module.c src/another.c"/* src/libretro.c: render_dual_screen() */Project-specific conventions & patterns

```

1. STIC outputs 352x224 BGR game frame- Libretro-first structure: public API is through `src/libretro.c`. Changes to module APIs must preserve libretro callback usage (Video, Audio, InputPoll, etc.).

## Project-Specific Conventions

2. 2x pixel doubling → 704x448 (placed at top of output buffer)- Single-file responsibilities: each hardware component is a single C file (cpu, stic, psg, ivoice); prefer edits within the owning file for local behavior.

### Serialization & Save States

- **Version constant**: `#define SERIALIZED_VERSION 0x4f544702` in `src/libretro.c`3. Load controller_base.png from <system>/freeintvds-overlays/- Serialization/versioning: save state uses a `SERIALIZED_VERSION` in `src/libretro.c`; when changing state layout update serialize/deserialize code and bump version.

- **When changing state layout**:

  1. Increment `SERIALIZED_VERSION`   - Dimensions: 446x620 pixels- BIOS paths: `retro_init()` calls `Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath)` and then expects `exec.bin` and `grom.bin` in that directory — don't hardcode paths.

  2. Update `retro_serialize()` to pack new fields

  3. Update `retro_unserialize()` to unpack and validate version   - Format: PNG with alpha channel

- **Per-module serialization**: Each component has `*Serialize()` / `*Unserialize()` pair (CP1610, STIC, PSG, ivoice)

- State includes: CPU registers, STIC state, PSG state, Intellivoice state, 16KB Memory[], global vars (`SR1`, `intv_halt`)   - Right-aligned in overlay section**Overlay conventions**:



### Libretro-First Architecture4. Load [ROM_NAME].png game-specific overlay- Each game can have a custom numeric pad overlay PNG in an `overlays` subfolder relative to the ROM directory.

- **Public API boundary**: All module interactions flow through `src/libretro.c` callbacks

- **Stability guarantee**: Do NOT change module function signatures without updating libretro glue layer   - Dimensions: 370x600 pixels- Overlay PNG filenames must match the ROM filename without extension (e.g., `overlays/Space Armada (USA).png`).

- **Input processing**: `joypad0[]` / `joypad1[]` arrays (20 elements each) built in `retro_run()` from libretro input, passed to `setControllerInput()` in `controller.c`

- **Video contract**: 704x1068 ARGB texture (32-bit, little-endian), one frame per `retro_run()` call   - Centered horizontally within 446px width- **Current implementation**: PNG detection and file opening works; PNG decompression in progress.

- **Audio contract**: Stereo PCM samples at 44.1 kHz via `audio_cb()`

   - Fallback: default.png if ROM-specific not found- Overlays render in right screen (256x224) while game renders in left screen (352x224).

### Overlay PNG System

- **File paths**: `<system>/freeintvds-overlays/controller_base.png` (446x620) + `<ROM_NAME>.png` (370x600 optional)5. Composite into 704x1068 ARGB buffer:- **File structure**: ROM at `/path/to/game.rom` looks for overlay at `/path/to/overlays/game.png`.

- **Fallback chain**: ROM-specific → generic `default.png` for each layer

- **PNG decoder**: stb_image.h (self-contained, no external dependencies)   - Black background (left 258 pixels of overlay area)- Handles RetroArch ZIP extraction to temporary directories with dynamic path resolution.

- **Pixel format**: Loaded as ARGB, alpha blending enabled for overlays

- **Path detection**: Case-sensitive on Linux/Mac, case-insensitive on Windows   - Game overlay (back layer, centered)



### Single-File Responsibility   - Controller base (front layer, with alpha blending)Integration points & external deps

Each hardware component is one `.c` file. Prefer local edits:

- **CPU timing changes** → `src/cp1610.c` in `CP1610Tick()` (consider downstream effects on `PSGTick()` / `ivoice_tk()` calls in `src/intv.c::exec()`)```- Depends on `libretro-common` under `src/deps/libretro-common` for portability helpers (file_path, compat). When adding features prefer these helpers.

- **Video rendering changes** → `src/stic.c` in sprite/background logic, then composite in `render_dual_screen()` (`src/libretro.c`)

- **Controller input mapping** → `src/controller.c` in `setControllerInput()` (called from `retro_run()`)- Build-time: compiler toolchain/flags are controlled by `Makefile`. Platform-specific blocks (Windows/MSVC, libnx, emscripten) are explicit there.

- **Audio synthesis** → `src/psg.c` / `src/ivoice.c` (called from `exec()` loop)

**Overlay path resolution**:- Runtime: integrates with libretro frontend via the callbacks defined in `libretro.h` and `libretro-core-options.h`.

### Common Symbol Names to Know

- `Memory[]` — 64KB address space (16KB accessible at a time via paging)- RetroArch system directory obtained in `retro_init()` via `RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY`

- `SR1` — CPU Status Register 1

- `intv_halt` — CPU halt flag- Looks for: `<system>/freeintvds-overlays/<ROM_NAME>.png`Quick editing rules for an AI code agent

- `joypad0[]`, `joypad1[]` — Input state arrays (20 elements: disc, buttons, analog)

- `frame[]` — Output framebuffer (704x1068 ARGB)- Fallback: `<system>/freeintvds-overlays/default.png`- Preserve existing public API and ABI across `src/*.c` unless the task explicitly updates save-state or libretro interfaces.



## Integration Points & External Dependencies- Handles ZIP extraction by parsing original ROM filename from path- When adding new files: update `Makefile.common` `SOURCES_C` list or `Makefile` `SOURCES_C` entries (the repo collects sources there).



### libretro-common Utilities (Preferred)- Update `link.T` only if symbols/exports change (rare).

Portability helpers in `src/deps/libretro-common/`:

- **File I/O**: `file/file_path.c` — path manipulation, file existence checks, path separators## Developer Workflows- If changing save-state, bump `SERIALIZED_VERSION` and update `retro_serialize()` / `retro_unserialize()` accordingly (see `src/libretro.c`).

- **Strings**: `compat_posix_string.c`, `string/stdstring.c` — cross-platform string operations

- **Encoding**: `encodings/encoding_utf.c` — UTF-8 handling

- **Math compat**: Various `compat_*.c` files for MSVC/non-POSIX systems

### Windows Build (MinGW-w64 Recommended)**Examples (short)**

Use these instead of OS-specific APIs for portability.

```powershell- To find where an input mapping is done: `src/libretro.c` -> `retro_run()` builds `joypad0[]`/`joypad1[]` and calls `setControllerInput()` in `src/controller.c`.

### Libretro Callback Functions (All in `src/libretro.c`)

- `video_cb(const void *data)` — Send frame buffer (704x1068 pixels)# First time: Install MinGW-w64, add bin/ to PATH, restart VS Code- To change CPU timing: edit `CP1610Tick()` in `src/cp1610.c` and consider downstream effects on `PSGTick()`/`ivoice_tk()` calls in `src/intv.c`.

- `audio_cb(int16_t left, int16_t right)` — Send stereo audio sample

- `InputPoll()` — Signal frontend to update input state- Dual-screen rendering: `test_function()` in `src/libretro.c` combines STIC output (left) with overlay (right) into single 608x224 buffer.

- `int InputState(unsigned port, unsigned device, unsigned index, unsigned id)` — Query current input

- `bool Environ(unsigned cmd, void *data)` — Query frontend environment (system dir, options, etc.)mingw32-make clean              # Clean object files- PNG overlay loading: `load_overlay_for_rom()` in `src/libretro.c` handles path resolution and file detection.



## Quick Reference: Common Tasksmingw32-make                    # Build → freeintvds_libretro.dll (dual-screen)



**Find where controller input affects game**:mingw32-make DEBUG=1            # Build with debug symbols## Current development status

- `retro_run()` → builds `joypad0[]` → `setControllerInput(port, state)` → updates hardware state

mingw32-make freeintv           # Build original single-screen variant- **Dual-screen display**: ✅ Working (608x224 combined resolution)

**Add a new core option**:

- Edit `src/libretro_core_options.h` (define option) + `check_variables()` in `src/libretro.c` (apply it)- **PNG file detection**: ✅ Working (finds overlays relative to ROM path)



**Fix CPU timing issue**:# To build with extra sources- **PNG loading**: ✅ Working (stb_image loads PNG data with correct dimensions)

- Edit `CP1610Tick()` in `src/cp1610.c` → check downstream `PSGTick()` / `ivoice_tk()` calls in `exec()` loop

mingw32-make EXTRA_SOURCES=src/my_feature.c- **Build system**: ✅ Working (`mingw32-make` produces `freeintvds_libretro.dll`)

**Change display resolution**:

- Modify constants in `src/libretro.c` (704, 1068, 448, 620) + `retro_get_system_av_info()` geometry```- **Core identification**: ✅ Working (identifies as "FreeIntvDS" when `FREEINTV_DS` defined)



**Debug PNG rendering**:- **PNG display rendering**: ❌ **CURRENT ISSUE** - PNG loads correctly but displays as "color static"

- PNG detection & loading: `src/libretro.c` (paths, stb_image calls)

- Pixel composition: `render_dual_screen()` (ARGB conversion, alpha blending)### Cross-Platform Builds



**Common build failures**:```bash## Active development issue (October 14, 2025)

- **"mingw32-make: command not found"** → MinGW not in PATH; restart VS Code after install

- **"exec.bin not found"** → Copy BIOS files to RetroArch system directory (names are case-sensitive on non-Windows)make platform=osx               # macOS**Problem**: PNG overlay files load successfully (correct dimensions, pixel count) but display as "color static" instead of coherent images.

- **Symbol export errors** → Only edit `link.T` if changing public function exports (rare)

make platform=libnx             # Nintendo Switch

## Current Development Status

make platform=unix              # Linux generic**Confirmed working**:

### ✅ Confirmed Working (October 2025)

- Vertical dual-screen display (704x1068)make platform=android           # Android (uses jni/Android.mk instead)- File detection: `overlays/[ROM_NAME].png` files found correctly

- PNG overlay loading (controller base + game-specific)

- Layered alpha blending```- PNG loading: stb_image loads 256x224 PNG with 57,344 pixels as expected

- 2x pixel upscaling (no artifacts)

- ZIP ROM support with overlay matching- Display pipeline: Corner color tests confirm 608x224 dual-screen rendering works

- Fallback overlays (default.png)

- Clean Windows build (MinGW-w64)### Local Testing (Manual)- Build chain: Clean compilation with MinGW-w64

- Save state serialization

No automated tests — validation is manual:

### 🔄 In Progress

- Android NDK build and hardware testing1. Copy compiled `.dll` / `.so` to RetroArch cores directory**Investigation needed**:

- Touch input mapping (future)

2. Ensure `exec.bin` and `grom.bin` in RetroArch system directory- Pixel processing loop: RGB→ARGB conversion may have bugs

### 📋 Best Practices for AI Agents

- **Start with STATUS.md** to find current blockers and feature list3. Load ROM in RetroArch- Pixel data extraction: Verify PNG RGB values are read correctly

- **Preserve serialization compatibility** — don't change state layout without version bump + validation

- **Test in RetroArch** before committing changes4. Verify 704x1068 display with game + overlays rendering correctly- Debug statistics: Variable scope issues prevent detailed pixel processing logs

- **Never hardcode paths** — use libretro `Environ()` or `libretro-common` file_path helpers

- **Comment complex dual-screen logic** — rendering is intricate

- **Verify module tick order** — if changing emulation loop, ensure all subsystem callbacks still invoked

- **Prefer libretro-common helpers** over OS-specific APIs### Adding New Source Files**Next session priorities**:


Update `Makefile.common` `SOURCES_C` list, or pass at build time:1. Create controlled test PNG with known content (solid colors, simple patterns)

```bash2. Debug pixel data extraction from PNG before ARGB conversion

mingw32-make EXTRA_SOURCES="src/new_module.c src/another.c"3. Fix variable scope issues to restore pixel processing statistics

```4. Verify ARGB conversion logic in pixel processing loop



## Project-Specific Conventions**Test setup ready**:

- Working core: `freeintvds_libretro.dll`

### Serialization & Save States- Test ROM: `open-content/4-Tris/4-tris.rom`

- **Version constant**: `#define SERIALIZED_VERSION 0x4f544702` in `src/libretro.c`- Overlay directory: `open-content/4-Tris/overlays/` (created, ready for test PNG)

- **When changing state layout**: - Debug logging: Console output and `freeintvds_debug.txt` functional

  1. Increment `SERIALIZED_VERSION`

  2. Update `retro_serialize()` to pack new fields## What to ask the maintainers

  3. Update `retro_unserialize()` to unpack and validate version- Preferred target platforms to test (Linux, Windows, Android, libnx?).

- **Per-module serialization**: Each component has `*Serialize()` / `*Unserialize()` pair:- Any planned API/serialization changes that the agent should avoid touching.

  - `CP1610Serialize()` / `CP1610Unserialize()`

  - `STICSerialize()` / `STICUnserialize()`If anything above is unclear or you want the file to emphasize other areas (tests, CI, Android packaging), say which sections to expand.

  - `PSGSerialize()` / `PSGUnserialize()`
  - `ivoiceSerialize()` / `ivoiceUnserialize()`
- State includes: CPU registers, STIC state, PSG state, Intellivoice state, 16KB Memory[], global vars (`SR1`, `intv_halt`)

### Libretro-First Architecture
- **Public API boundary**: All module interactions flow through `src/libretro.c` callbacks
- **Stability guarantee**: Do NOT change module function signatures without updating libretro glue layer
- **Input processing**: `joypad0[]` / `joypad1[]` arrays (20 elements each) built in `retro_run()` from libretro input, passed to `setControllerInput()` in `controller.c`
- **Video contract**: 704x1068 ARGB texture (32-bit, little-endian), one frame per `retro_run()` call
- **Audio contract**: Stereo PCM samples at 44.1 kHz via `audio_cb()`

### Overlay PNG System
- **File paths**: `<system>/freeintvds-overlays/controller_base.png` (446x620) + `<ROM_NAME>.png` (370x600 optional)
- **Fallback chain**: ROM-specific → generic `default.png` for each layer
- **PNG decoder**: stb_image.h (self-contained, no external dependencies)
- **Pixel format**: Loaded as ARGB, alpha blending enabled for overlays
- **Path detection**: Case-sensitive on Linux/Mac, case-insensitive on Windows

### Single-File Responsibility
Each hardware component is one `.c` file. Prefer local edits:
- **CPU timing changes** → `src/cp1610.c` in `CP1610Tick()` (consider downstream effects on `PSGTick()` / `ivoice_tk()` calls in `src/intv.c::exec()`)
- **Video rendering changes** → `src/stic.c` in sprite/background logic, then composite in `render_dual_screen()` (`src/libretro.c`)
- **Controller input mapping** → `src/controller.c` in `setControllerInput()` (called from `retro_run()`)
- **Audio synthesis** → `src/psg.c` / `src/ivoice.c` (called from `exec()` loop)

### Common Symbol Names to Know
- `Memory[]` — 64KB address space (16KB accessible at a time via paging)
- `SR1` — CPU Status Register 1
- `intv_halt` — CPU halt flag
- `joypad0[]`, `joypad1[]` — Input state arrays (20 elements: disc, buttons, analog)
- `frame[]` — Output framebuffer (704x1068 ARGB)

## Integration Points & External Dependencies

### libretro-common Utilities (Preferred)
Portability helpers in `src/deps/libretro-common/`:
- **File I/O**: `file/file_path.c` — path manipulation, file existence checks, path separators
- **Strings**: `compat_posix_string.c`, `string/stdstring.c` — cross-platform string operations
- **Encoding**: `encodings/encoding_utf.c` — UTF-8 handling
- **Math compat**: Various `compat_*.c` files for MSVC/non-POSIX systems

Use these instead of OS-specific APIs for portability.

### Libretro Callback Functions (All in `src/libretro.c`)
- `video_cb(const void *data)` — Send frame buffer (704x1068 pixels)
- `audio_cb(int16_t left, int16_t right)` — Send stereo audio sample
- `InputPoll()` — Signal frontend to update input state
- `int InputState(unsigned port, unsigned device, unsigned index, unsigned id)` — Query current input
- `bool Environ(unsigned cmd, void *data)` — Query frontend environment (system dir, options, etc.)

## Quick Reference: Common Tasks

**Find where controller input affects game**:
- `retro_run()` → builds `joypad0[]` → `setControllerInput(port, state)` → updates hardware state

**Add a new core option**:
- Edit `src/libretro_core_options.h` (define option) + `check_variables()` in `src/libretro.c` (apply it)

**Fix CPU timing issue**:
- Edit `CP1610Tick()` in `src/cp1610.c` → check downstream `PSGTick()` / `ivoice_tk()` calls in `exec()` loop

**Change display resolution**:
- Modify constants in `src/libretro.c` (704, 1068, 448, 620) + `retro_get_system_av_info()` geometry

**Debug PNG rendering**:
- PNG detection & loading: `src/libretro.c` (paths, stb_image calls)
- Pixel composition: `render_dual_screen()` (ARGB conversion, alpha blending)

**Common build failures**:
- **"mingw32-make: command not found"** → MinGW not in PATH; restart VS Code after install
- **"exec.bin not found"** → Copy BIOS files to RetroArch system directory (names are case-sensitive on non-Windows)
- **Symbol export errors** → Only edit `link.T` if changing public function exports (rare)

## Current Development Status

### ✅ Confirmed Working (October 2025)
- Vertical dual-screen display (704x1068)
- PNG overlay loading (controller base + game-specific)
- Layered alpha blending
- 2x pixel upscaling (no artifacts)
- ZIP ROM support with overlay matching
- Fallback overlays (default.png)
- Clean Windows build (MinGW-w64)
- Save state serialization

### 🔄 In Progress
- Android NDK build and hardware testing
- Touch input mapping (future)

### 📋 Best Practices for AI Agents
- **Start with STATUS.md** to find current blockers and feature list
- **Preserve serialization compatibility** — don't change state layout without version bump + validation
- **Test in RetroArch** before committing changes
- **Never hardcode paths** — use libretro `Environ()` or `libretro-common` file_path helpers
- **Comment complex dual-screen logic** — rendering is intricate
- **Verify module tick order** — if changing emulation loop, ensure all subsystem callbacks still invoked
- **Prefer libretro-common helpers** over OS-specific APIs
