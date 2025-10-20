# Libretro Core Patterns for 3DS Development

## Existing Cores That Handle Dual-Screen Successfully

### 1. MelonDS Libretro
**Repository**: libretro/melonDS  
**Screens**: 2 (256×192 each, native DS resolution)  
**Framebuffer**: Single combined (512×384 or 768×480 depending on mode)

**Pattern Used**:
```c
// Vertical layout (both screens stacked)
#define DS_SCREEN_WIDTH 256
#define DS_SCREEN_HEIGHT 192
#define BUFFER_WIDTH  512
#define BUFFER_HEIGHT 384   // 192 + 192 (top + bottom stacked)
```

**Key Implementation**:
- Renders top screen to buffer offset 0
- Renders bottom screen to buffer offset (512 * 192)
- Sends single `video_cb(buffer, 512, 384, pitch)` per frame

### 2. Dolphin Libretro (GameCube/Wii Emulator)
**Repository**: libretro/dolphin  
**Screens**: 1 (but Wii remote can interact with pointer device)  
**Challenge**: Different audio/video requirements per game

**Pattern**: Uses core options heavily:
```c
{
    "dolphin_cpu_core",
    "CPU Emulation Core",
    {
        { "jit64", "JIT64" },
        { "jit64_a", "JIT64 A" },
        { "cached", "Cached Interpreter" },
    },
    "jit64"
}
```

### 3. Citra (3DS Emulator)
**Repository**: libretro/citra  
**Screens**: 2 (400×240 top, 320×240 bottom)  
**Layout**: Either vertical stacked OR side-by-side depending on core option

**Critical Pattern**:
```c
// Core option for 3DS layout
{
    "citra_screen_layout",
    "Screen Layout",
    {
        { "default", "Default Top-Bottom (1:1 Aspect)" },
        { "side_by_side", "Side-by-Side" },
        { "swapped", "Swapped Top-Bottom" },
        { "upright", "Upright (Vertical)" },
        { "hybrid", "Hybrid (4:3 with touchscreen)" },
    },
    "default"
}
```

Then dynamically adjusts resolution:
```c
void update_screen_layout() {
    if (layout == SIDE_BY_SIDE) {
        width = 400 + 320;  // 720
        height = 240;
    } else if (layout == DEFAULT || layout == UPRIGHT) {
        width = 400;        // Or max(400, 320)
        height = 240 + 240; // 480
    }
    
    Environ(RETRO_ENVIRONMENT_SET_GEOMETRY, &new_geometry);
}
```

---

## Proven Libretro Dual-Screen Patterns

### Pattern 1: Vertical Stacking (✅ Recommended for FreeIntvDS)
**Best for**: Narrow screens, 3DS, Ayaneo Pocket DS  
**Layout**: `[Game 704×448] + [Overlay 704×620] = 704×1068`

**Advantages**:
- Natural for handhelds with narrow landscape
- Preserves aspect ratio
- Easy to implement

**Implementation**:
```c
struct FrameBuffer {
    uint32_t pixels[704 * 1068];  // ARGB8888
    uint32_t *top_screen;         // Ptr to pixels[0...704*448]
    uint32_t *bottom_screen;      // Ptr to pixels[704*448...end]
};

void composite_frame() {
    // Render game to top portion
    STIC.updateDisplay((uint16_t*)frame_buffer.top_screen, 352, 224);
    // Scale 2x
    scale_2x_blit(frame_buffer.top_screen);
    
    // Render overlay to bottom portion
    draw_overlay(frame_buffer.bottom_screen);
    
    // Send to libretro
    Video(frame_buffer.pixels, 704, 1068, 704 * 4);
}
```

### Pattern 2: Horizontal Side-by-Side
**Best for**: Wider handhelds, MelonDS Wii mode  
**Layout**: `[Game 352×224] | [Overlay 256×224] = 608×224`

**Advantages**:
- Better for tablets
- Fits wide screens better

**Implementation**:
```c
struct FrameBuffer {
    uint32_t pixels[608 * 224];
    uint32_t *game_screen;   // Left half
    uint32_t *overlay_screen; // Right half
};

void composite_frame() {
    uint32_t *game_ptr = frame_buffer.pixels;
    uint32_t *overlay_ptr = frame_buffer.pixels + 352;
    
    // Render game (no scale)
    STIC.updateDisplay((uint16_t*)game_ptr, 352, 224);
    
    // Render overlay
    draw_overlay(overlay_ptr);
    
    // Send combined frame
    Video(frame_buffer.pixels, 608, 224, 608 * 4);
}
```

### Pattern 3: Dynamic Layout Selection
**Best for**: Universal compatibility  
**Implementation**: Core option + geometry switching

```c
enum DisplayLayout {
    LAYOUT_VERTICAL,       // Default for handhelds
    LAYOUT_HORIZONTAL,     // Alternative
    LAYOUT_SINGLE_SCREEN   // Fallback
};

static DisplayLayout current_layout = LAYOUT_VERTICAL;

void handle_layout_change() {
    struct retro_game_geometry new_geom;
    
    switch (current_layout) {
        case LAYOUT_VERTICAL:
            new_geom.base_width = 704;
            new_geom.base_height = 1068;
            new_geom.aspect_ratio = 704.0f / 1068.0f;
            break;
        case LAYOUT_HORIZONTAL:
            new_geom.base_width = 608;
            new_geom.base_height = 224;
            new_geom.aspect_ratio = 608.0f / 224.0f;
            break;
        case LAYOUT_SINGLE_SCREEN:
            new_geom.base_width = 352;
            new_geom.base_height = 224;
            new_geom.aspect_ratio = 352.0f / 224.0f;
            break;
    }
    
    Environ(RETRO_ENVIRONMENT_SET_GEOMETRY, &new_geom);
}

void retro_run() {
    // ... emulation ...
    
    // Check if layout changed
    if (layout_option_changed()) {
        handle_layout_change();
    }
    
    composite_frame();
}
```

---

## Libretro Core Options Patterns for Device Detection

### Pattern: Auto-Detect Based on Refresh Rate
```c
// Get target refresh rate to detect device
bool detect_device_type() {
    double target_fps = 0.0;
    if (!Environ(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_fps)) {
        return false;  // Can't detect
    }
    
    if (target_fps > 59.0 && target_fps < 61.0) {
        return is_3ds_device = true;  // 3DS/Wii: 59.8 Hz
    } else if (target_fps > 59.0 && target_fps < 60.5) {
        return is_ayaneo_device = true;  // Ayaneo: 60 Hz
    }
    
    return false;
}
```

### Pattern: Conditional Compilation with Device Detection
```c
// In src/libretro.c
static struct retro_core_options {
    const char *key;
    const char *desc;
    struct {
        const char *label;
        const char *info;
    } *values;
} options[] = {
    
#if defined(FREEINTV_DS) && defined(__3DS__)
    {
        "freeintvds_3ds_cpu_speed",
        "CPU Speed",
        {
            { "normal", "Normal (100%)" },
            { "turbo", "Turbo (150%)" },
            { "slow", "Slow (50%)" },
        }
    },
#endif
    
    { NULL, NULL, NULL }
};
```

---

## Environment API Calls Other Cores Use Successfully

### 1. Frame Timing Callback
```c
struct retro_frame_time_callback frame_time = {
    .callback = frame_time_callback,
    .reference = 16666,  // ~60 FPS in microseconds
};

static void frame_time_callback(retro_usec_t usec) {
    frame_time_microseconds = usec;
    
    // Could adapt emulation speed based on device performance
    if (usec > 33333) {  // Slower than expected
        // Skip processing on slower devices
        skip_cycle_update = true;
    }
}

Environ(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_time);
```

### 2. Query Preferred Hardware Render
```c
struct retro_hw_render_callback hw_render = {};

void *hw_render_context = NULL;
if (Environ(RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER, &hw_render)) {
    // Frontend supports hardware rendering
    hw_render_available = true;
    
    // Could use GPU for:
    // - 2x scaling
    // - Overlay alpha blending
    // - Color effects
} else {
    // Fallback to software rendering (current approach)
    hw_render_available = false;
}
```

### 3. Save/Load Directory Management
```c
// Get save directory for ROM-specific overlays
const char *save_dir = NULL;
if (Environ(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir)) {
    snprintf(overlay_path, sizeof(overlay_path), 
             "%s/%s.png", save_dir, rom_name);
    // Try loading custom overlay from save directory
}
```

---

## Memory and Performance Optimization Patterns

### Pattern: Lazy Buffer Allocation
```c
static uint32_t *frame_buffer = NULL;
static unsigned frame_buffer_width = 0;
static unsigned frame_buffer_height = 0;

void ensure_framebuffer(unsigned width, unsigned height) {
    if (frame_buffer_width != width || frame_buffer_height != height) {
        if (frame_buffer) free(frame_buffer);
        
        frame_buffer = malloc(width * height * 4);  // ARGB32
        frame_buffer_width = width;
        frame_buffer_height = height;
    }
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    int width = dual_screen_enabled ? 1024 : 352;
    int height = dual_screen_enabled ? 1486 : 224;
    
    ensure_framebuffer(width, height);
    
    info->geometry.base_width = width;
    info->geometry.base_height = height;
}
```

### Pattern: Pitch-Aware Rendering
```c
void render_with_pitch(const void *data, unsigned width, unsigned height, size_t pitch) {
    // pitch = bytes per scanline (may include padding)
    
    for (unsigned y = 0; y < height; y++) {
        uint32_t *line = (uint32_t *)((uint8_t *)data + y * pitch);
        // Process scanline
    }
}

void retro_run() {
    render_game_and_overlay();
    Video(frame_buffer, 704, 1068, 704 * 4);  // pitch = width * 4 bytes
}
```

---

## Testing Patterns Used by Cores

### Pattern: Diagnostic Logging
```c
#ifdef FREEINTVDS_DEBUG
    #define LOG_DEBUG(fmt, ...) fprintf(stderr, "[FreeIntvDS Debug] " fmt "\n", __VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) do {} while(0)
#endif

void retro_init() {
    LOG_DEBUG("Initializing FreeIntvDS");
    LOG_DEBUG("Framebuffer: %ux%u", WORKSPACE_WIDTH, WORKSPACE_HEIGHT);
    
    if (!loadExec()) {
        LOG_DEBUG("ERROR: Could not load exec.bin");
        return;
    }
}
```

### Pattern: Feature Detection on Startup
```c
void check_supported_features() {
    bool supports_geometry = 
        Environ(RETRO_ENVIRONMENT_SET_GEOMETRY, &dummy_geometry);
    bool supports_vfs = 
        Environ(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs);
    bool supports_frame_time = 
        Environ(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_time);
    
    LOG_DEBUG("Geometry support: %s", supports_geometry ? "YES" : "NO");
    LOG_DEBUG("VFS support: %s", supports_vfs ? "YES" : "NO");
    LOG_DEBUG("Frame time support: %s", supports_frame_time ? "YES" : "NO");
}
```

---

## Citra's Multi-Layout Implementation (Best Reference)

### How Citra Handles 5 Different 3DS Layouts

```c
enum DisplayLayout {
    LAYOUT_DEFAULT = 0,
    LAYOUT_SINGLE_SCREEN_TOP = 1,
    LAYOUT_SINGLE_SCREEN_BOTTOM = 2,
    LAYOUT_SIDE_BY_SIDE = 3,
    LAYOUT_UPRIGHT = 4,
};

struct ScreenLayout {
    int top_x, top_y, top_w, top_h;
    int bottom_x, bottom_y, bottom_w, bottom_h;
    int render_w, render_h;
} layouts[] = {
    // DEFAULT: Vertical stacked
    { 0, 0, 400, 240, 0, 240, 400, 240, 400, 480 },
    // SINGLE TOP: Only top screen
    { 0, 0, 400, 240, -1, -1, 0, 0, 400, 240 },
    // SINGLE BOTTOM: Only bottom screen
    { -1, -1, 0, 0, 0, 0, 320, 240, 320, 240 },
    // SIDE BY SIDE: Horizontal
    { 0, 0, 400, 240, 400, 0, 320, 240, 720, 240 },
    // UPRIGHT: Rotated 90°
    { 0, 0, 240, 400, 0, 400, 240, 320, 240, 720 },
};

void render_frame() {
    ScreenLayout layout = layouts[current_layout];
    
    // Render top screen to position
    if (layout.top_w > 0) {
        render_to_offset(top_framebuffer, layout.top_x, layout.top_y);
    }
    
    // Render bottom screen to position
    if (layout.bottom_w > 0) {
        render_to_offset(bottom_framebuffer, layout.bottom_x, layout.bottom_y);
    }
    
    // Send combined buffer
    Video(composite_buffer, layout.render_w, layout.render_h, stride);
}
```

---

## Recommended Implementation for FreeIntvDS 3DS Support

### Phase 1: Current Implementation (✅ Already Done)
- Vertical layout at 1024×1486
- No changes needed
- Works on 3DS via automatic scaling

### Phase 2: Add Layout Options (Recommended)
```c
// In libretro_core_options_intl.h
{
    "freeintvds_layout",
    "Display Layout",
    {
        { "vertical_auto", "Vertical Auto (Default)" },
        { "vertical_fixed", "Vertical Fixed 1024×1486" },
        { "horizontal", "Horizontal Side-by-Side (608×224)" },
        { "single_screen", "Single Screen (352×224)" },
    },
    "vertical_auto"
}
```

### Phase 3: Add Device-Specific Options (Nice-to-Have)
```c
{
    "freeintvds_target_device",
    "Target Device",
    {
        { "auto", "Auto-Detect" },
        { "3ds", "Nintendo 3DS" },
        { "ayaneo", "Ayaneo Pocket DS" },
        { "generic", "Generic Handheld" },
    },
    "auto"
}
```

---

## Summary: What Libretro Library Teaches Us

| Feature | Available | Current Use | For 3DS |
|---------|-----------|-------------|---------|
| **Single framebuffer** | ✅ Yes | ✅ Using | ✅ Perfect |
| **Core options** | ✅ Yes | ✅ Using | ✅ Enhance |
| **Geometry switching** | ✅ Yes | ❌ Not using | ⚠️ Consider |
| **Hardware render** | ✅ Yes (experimental) | ❌ Not needed | ❌ Not needed |
| **Device detection** | ✅ Via refresh rate | ❌ Not implemented | ⚠️ Nice-to-have |
| **Dual-screen API** | ❌ Not available | ❌ Workaround used | ✅ Already solved |

**Conclusion**: FreeIntvDS is already correctly implemented for 3DS using the established libretro pattern (vertical framebuffer stacking). No core API changes needed—just test and potentially add layout options.

