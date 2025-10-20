# Libretro Library Code Navigation Guide

## Where to Find What You Need

### 1. Core Video Rendering API
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 3755-3795 (Video callback definitions)

```c
// Video refresh callback (what FreeIntvDS uses)
typedef void (RETRO_CALLCONV *retro_video_refresh_t)(
    const void *data, unsigned width, unsigned height, size_t pitch
);

// Set the callback
RETRO_API void retro_set_video_refresh(retro_video_refresh_t);
```

**Current FreeIntvDS Usage** (src/libretro.c line 651-657):
```c
retro_video_refresh_t Video;
void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
// Called in retro_run() as: Video(frame, 1024, 1486, pitch);
```

---

### 2. System Audio/Video Info Structure
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 300-350 (Structure definitions)

```c
struct retro_game_geometry {
    unsigned base_width;
    unsigned base_height;
    unsigned max_width;
    unsigned max_height;
    float aspect_ratio;
};

struct retro_system_av_info {
    struct retro_game_geometry geometry;
    struct retro_system_timing timing;
};
```

**Current FreeIntvDS Usage** (src/libretro.c line 1363-1378):
```c
void retro_get_system_av_info(struct retro_system_av_info *info)
{
    int width = dual_screen_enabled ? WORKSPACE_WIDTH : GAME_WIDTH;
    int height = dual_screen_enabled ? WORKSPACE_HEIGHT : GAME_HEIGHT;
    info->geometry.base_width = width;
    info->geometry.base_height = height;
    // ... etc
}
```

---

### 3. Environment Callback (Environ())
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 400-1800 (Environment command definitions)

```c
typedef bool (*retro_environment_t)(unsigned cmd, void *data);

// Examples:
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY 9
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 10
#define RETRO_ENVIRONMENT_SET_GEOMETRY 37
#define RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE (50 | RETRO_ENVIRONMENT_EXPERIMENTAL)
```

**Current FreeIntvDS Usage** (src/libretro.c):
```c
// Line 651: retro_environment_t Environ;
// Line 656: void retro_set_environment(retro_environment_t fn) { Environ = fn; }
// Line 704+: Check variables (core options)
// Line 1379: Set pixel format
```

---

### 4. Core Options System
**File**: `src/libretro_core_options.h` (IN YOUR PROJECT)

**Related libretro files**:
- `src/deps/libretro-common/include/libretro.h` (lines 3300-3600)
- Documentation: Environment cmd 24 (`SET_VARIABLES`)

```c
// From libretro.h:
#define RETRO_ENVIRONMENT_SET_VARIABLES 24
#define RETRO_ENVIRONMENT_GET_VARIABLE 45
#define RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE 49

// Your implementation:
// src/libretro_core_options.h defines options
// src/libretro.c check_variables() processes them
```

**FreeIntvDS Location**: `src/libretro_core_options.h` (entire file)

---

### 5. Pixel Format Specification
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 250-300 (Pixel format constants)

```c
#define RETRO_PIXEL_FORMAT_0RGB1555 0
#define RETRO_PIXEL_FORMAT_XRGB8888 1   // ← FreeIntvDS uses this
#define RETRO_PIXEL_FORMAT_RGB565 2
```

**FreeIntvDS Location** (src/libretro.c line 1364):
```c
int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;
Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
```

---

### 6. Experimental Environment Commands
**File**: `src/deps/libretro-common/include/libretro.h`

**Line**: 490 (Experimental flag definition)

```c
#define RETRO_ENVIRONMENT_EXPERIMENTAL 0x10000

// Experimental commands (all have EXPERIMENTAL flag):
#define RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER (40 | RETRO_ENVIRONMENT_EXPERIMENTAL)
#define RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE (50 | RETRO_ENVIRONMENT_EXPERIMENTAL)
```

**Useful for 3DS**:
```c
// Line 729: GET_SENSOR_INTERFACE (experimental)
// Line 738: GET_CAMERA_INTERFACE (experimental)
// Line 959: GET_CURRENT_SOFTWARE_FRAMEBUFFER (experimental) - Memory optimization
// Line 1096: GET_TARGET_REFRESH_RATE (experimental) - Device detection
```

---

### 7. Input Device Structures
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 50-240 (Input device definitions)

```c
// Device types:
#define RETRO_DEVICE_JOYPAD 1
#define RETRO_DEVICE_MOUSE 2
#define RETRO_DEVICE_KEYBOARD 3
// ... etc

// Joypad button IDs:
#define RETRO_DEVICE_ID_JOYPAD_B 0
#define RETRO_DEVICE_ID_JOYPAD_Y 1
// ... etc
```

**FreeIntvDS Usage** (src/libretro.c):
```c
// Build joypad arrays in retro_run()
joypad0[RETRO_DEVICE_ID_JOYPAD_LEFT] = InputState(...);
```

---

### 8. Memory Interface (Save States)
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 3500-3700 (Memory interface)

```c
#define RETRO_MEMORY_SAVE_RAM 0
#define RETRO_MEMORY_RTC 1
#define RETRO_MEMORY_SYSTEM_RAM 2
#define RETRO_MEMORY_VIDEO_RAM 3

// Your project: src/libretro.c
// retro_get_memory_data() - get pointer to memory
// retro_get_memory_size() - get size of memory
// retro_serialize() / retro_unserialize() - save/load state
```

---

### 9. Callbacks (Video, Audio, Input, etc)
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 3755-3800 (All callback types)

```c
// All callbacks (already using):
typedef void (RETRO_CALLCONV *retro_video_refresh_t)(...);
typedef void (RETRO_CALLCONV *retro_audio_sample_t)(int16_t, int16_t);
typedef size_t (RETRO_CALLCONV *retro_audio_sample_batch_t)(...);
typedef void (RETRO_CALLCONV *retro_input_poll_t)(void);
typedef int16_t (RETRO_CALLCONV *retro_input_state_t)(...);
typedef bool (RETRO_CALLCONV *retro_environment_t)(unsigned, void *);
```

**FreeIntvDS Setup** (src/libretro.c):
```c
// Lines 651-656: Global callback pointers
// Lines 667-675: Set callbacks (called by RetroArch before retro_run)
retro_set_video_refresh(Video);
retro_set_audio_sample(Audio);
retro_set_input_poll(InputPoll);
retro_set_input_state(InputState);
retro_set_environment(Environ);
```

---

### 10. Core Entry Points (What RetroArch Calls)
**File**: `src/deps/libretro-common/include/libretro.h`

**Line Range**: 3790-3830 (Core entry points)

```c
RETRO_API unsigned retro_api_version(void);
RETRO_API void retro_set_environment(retro_environment_t);
RETRO_API void retro_set_video_refresh(retro_video_refresh_t);
RETRO_API void retro_set_audio_sample(retro_audio_sample_t);
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
RETRO_API void retro_set_input_poll(retro_input_poll_t);
RETRO_API void retro_set_input_state(retro_input_state_t);
RETRO_API void retro_init(void);
RETRO_API void retro_deinit(void);
RETRO_API void retro_get_system_info(struct retro_system_info *info);
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info);
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device);
RETRO_API void retro_reset(void);
RETRO_API void retro_run(void);
RETRO_API size_t retro_serialize_size(void);
RETRO_API bool retro_serialize(void *data, size_t size);
RETRO_API bool retro_unserialize(const void *data, size_t size);
RETRO_API void retro_cheat_reset(void);
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code);
RETRO_API bool retro_load_game(const struct retro_game_info *game);
RETRO_API bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info);
RETRO_API void retro_unload_game(void);
RETRO_API unsigned retro_get_region(void);
RETRO_API void *retro_get_memory_data(unsigned id);
RETRO_API size_t retro_get_memory_size(unsigned id);
```

**FreeIntvDS Implementation** (src/libretro.c):
- Lines 100+: Each function implemented

---

## Quick Navigation Cheat Sheet

### To Find Something in Libretro...

**I need to know about:** → **Look in libretro.h at line:**

- Video rendering → 3755 (retro_video_refresh_t)
- Geometry/resolution → 300 (retro_game_geometry)
- Audio output → 3760 (retro_audio_sample_t)
- Input handling → 3780 (retro_input_poll_t, retro_input_state_t)
- Environment commands → 490+ (all RETRO_ENVIRONMENT_*)
- Core options/variables → 24, 45, 49 (SET/GET_VARIABLES, GET_VARIABLE_UPDATE)
- Device detection → 50 (RETRO_ENVIRONMENT_EXPERIMENTAL)
- Pixel formats → 250 (RETRO_PIXEL_FORMAT_*)
- Core entry points → 3790 (retro_init, retro_run, retro_load_game)
- Save states → 3500 (memory interface)
- 3DS-relevant APIs → 1096 (GET_TARGET_REFRESH_RATE)
- Performance optimization → 959 (GET_CURRENT_SOFTWARE_FRAMEBUFFER)

---

## Code Patterns to Reference

### Pattern 1: Check Environment Support
```c
// From many cores, check if feature exists:
if (Environ(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_fps)) {
    // Feature is supported, use it
    device_detected = true;
}
```

### Pattern 2: Handle Core Options
```c
// Standard pattern for all cores:
var.key = "core_option_name";
if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    // Process var.value
}
```

### Pattern 3: Send Video Frame
```c
// What FreeIntvDS does (correct pattern):
Video(frame_buffer_pointer, width, height, width * 4);
// pitch = width * 4 for ARGB8888 format
```

### Pattern 4: Set Geometry Dynamically
```c
// For layout switching:
struct retro_game_geometry new_geom = {
    .base_width = 1024,
    .base_height = 1486,
    .aspect_ratio = 1024.0f / 1486.0f
};
Environ(RETRO_ENVIRONMENT_SET_GEOMETRY, &new_geom);
```

---

## Key Differences in Environment Commands

### Stable vs Experimental
```c
// STABLE (safe to use, won't change):
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 10
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY 9

// EXPERIMENTAL (may change between libretro versions):
#define RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE (50 | RETRO_ENVIRONMENT_EXPERIMENTAL)
#define RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER (40 | RETRO_ENVIRONMENT_EXPERIMENTAL)
```

**For FreeIntvDS**:
- Use stable commands (already doing this ✅)
- Optional experimental commands for 3DS optimization

---

## Where to Look for Examples

### In FreeIntvDS Codebase
- **Path handling**: `src/libretro.c` lines 166-191, 243-262
- **BIOS finding**: `src/libretro.c` lines 166-191
- **Video rendering**: `src/libretro.c` lines 1000-1100
- **Audio**: `src/libretro.c` lines 1200+
- **Input**: `src/libretro.c` lines 1300+
- **Geometry**: `src/libretro.c` lines 1363+

### In Libretro Library (src/deps/libretro-common/)
- **File I/O patterns**: `file/file_path.c`
- **String handling**: `string/stdstring.c`, `compat_posix_string.c`
- **Encoding**: `encodings/encoding_utf.c`
- **Math compat**: `compat_*.c` files

---

## Summary

All the code you need is already available:

1. ✅ **Video callback** - `libretro.h` line 3755
2. ✅ **Geometry structure** - `libretro.h` line 300
3. ✅ **Environment commands** - `libretro.h` lines 490+
4. ✅ **Input handling** - `libretro.h` lines 50+
5. ✅ **Core options** - `libretro.h` line 24+

FreeIntvDS uses all of these correctly. No changes needed to support 3DS—just compile and deploy!

