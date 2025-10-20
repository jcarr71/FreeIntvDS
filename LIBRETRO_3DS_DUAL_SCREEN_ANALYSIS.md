# Libretro Library Analysis: 3DS Dual-Screen Support

## Executive Summary

The **libretro API does NOT provide native dual-screen support**. However, there ARE existing patterns and environment variables that 3DS implementations use to achieve dual-screen rendering within libretro's single-framebuffer constraint.

### Key Finding
3DS cores (like MelonDS) handle dual-screen by:
1. **Rendering both screens into a single combined framebuffer** (just like we do for Ayaneo)
2. **Packing them vertically or horizontally** based on device requirements
3. **Using core options to let users choose layout** (vertical vs horizontal)

This is **exactly what FreeIntvDS does at 1024x1486**.

---

## Libretro Video Rendering Architecture

### Current Callback Model
```c
typedef void (RETRO_CALLCONV *retro_video_refresh_t)(
    const void *data,      // Pointer to framebuffer
    unsigned width,        // Framebuffer width
    unsigned height,       // Framebuffer height
    size_t pitch           // Bytes per scanline
);
```

**Limitation**: One framebuffer per `retro_run()` call. No separate screen callbacks.

### Geometry Management
```c
struct retro_game_geometry {
    unsigned base_width;      // Nominal width
    unsigned base_height;     // Nominal height
    unsigned max_width;       // Maximum width
    unsigned max_height;      // Maximum height
    float aspect_ratio;       // Aspect ratio
};

struct retro_system_av_info {
    struct retro_game_geometry geometry;
    struct retro_system_timing timing;
};
```

**Current FreeIntvDS Implementation**:
```c
void retro_get_system_av_info(struct retro_system_av_info *info)
{
    int width = dual_screen_enabled ? WORKSPACE_WIDTH : GAME_WIDTH;    // 1024 or 352
    int height = dual_screen_enabled ? WORKSPACE_HEIGHT : GAME_HEIGHT; // 1486 or 224
    
    info->geometry.base_width   = width;
    info->geometry.base_height  = height;
    info->geometry.max_width    = width;
    info->geometry.max_height   = height;
    info->geometry.aspect_ratio = ((float)width) / ((float)height);
}
```

---

## Environment Variables Useful for Multi-Display

### 1. `RETRO_ENVIRONMENT_SET_GEOMETRY` (Softer Resolution Change)
```c
#define RETRO_ENVIRONMENT_SET_GEOMETRY 37
/* const struct retro_game_geometry * */
```

**Purpose**: Change geometry WITHOUT requiring full driver reinitialization (unlike `SET_SYSTEM_AV_INFO`)

**Potential Use**:
- Allow users to toggle between layouts (vertical dual-screen vs horizontal vs single-screen)
- Called dynamically during `retro_run()` without heavy cost

**Currently Used?** ✅ Could be added to allow layout switching

---

### 2. `RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER` (Hardware Context)
```c
#define RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER 56
/* struct retro_hw_render_context_type * */
```

**Purpose**: Query if frontend supports hardware rendering (OpenGL, Vulkan, etc.)

**3DS Relevance**: 3DS has limited GPU, so frontends might prefer software rendering or specific GPU paths

**Currently Used?** ❌ Not in FreeIntvDS (using software rendering)

---

### 3. `RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER` (EXPERIMENTAL)
```c
#define RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER (40 | RETRO_ENVIRONMENT_EXPERIMENTAL)
/* struct retro_framebuffer * */
```

**Purpose**: Get direct access to frontend's framebuffer for rendering (vs copying)

**Benefit**: More efficient for 3DS with tight memory/CPU constraints

**Structure**:
```c
struct retro_framebuffer {
    void *data;           // Pointer to framebuffer
    unsigned width;       // Width in pixels
    unsigned height;      // Height in pixels
    size_t pitch;         // Bytes per scanline
    enum retro_pixel_format format;
    bool access_flags;    // ...
    bool memory_flags;    // ...
};
```

**Currently Used?** ❌ Not in FreeIntvDS

**Potential Benefit for 3DS**: Could reduce memory pressure

---

### 4. `RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE` (EXPERIMENTAL)
```c
#define RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE (47 | RETRO_ENVIRONMENT_EXPERIMENTAL)
/* bool * (enabled) */
```

**Purpose**: Check if audio/video rendering should be disabled (e.g., for fast-forward)

**3DS Relevance**: 3DS frontends might disable bottom screen rendering for performance

**Currently Used?** ❌ Not in FreeIntvDS

---

### 5. `RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE` (EXPERIMENTAL)
```c
#define RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE (43 | RETRO_ENVIRONMENT_EXPERIMENTAL)
```

**Purpose**: Advanced GPU context management

**3DS Relevance**: Could optimize for 3DS GPU if frontend supports it

**Currently Used?** ❌ Not in FreeIntvDS

---

## How 3DS RetroArch Handles Dual-Screen

### RetroArch 3DS Frontend Behavior

The **libretro 3DS frontend** does the actual dual-screen work:

1. **Receives 704x1068 texture** (or whatever resolution core provides)
2. **Frontend code routes pixels**:
   - Top portion → Top 3DS screen
   - Bottom portion → Bottom 3DS screen
3. **Handles scaling** to fit actual 3DS screen dimensions (320x240 top, 320x240 bottom)

### Example: What MelonDS 3DS Does

MelonDS on 3DS doesn't use separate framebuffers—it:
1. Renders DS screens into single buffer (512x384 or 768x480)
2. Sends to libretro as one texture
3. RetroArch 3DS frontend automatically maps portions to hardware screens

---

## Recommended Strategy for FreeIntvDS on 3DS

### Current Working Approach (1024x1486)
✅ This will work on 3DS! The 3DS libretro frontend will:
1. Receive 1024x1486 framebuffer
2. Automatically scale to fit both 3DS screens
3. Display top portion (game) on top screen
4. Display bottom portion (overlay) on bottom screen

### Optional Optimization: Add Core Option for Layout

```c
// In libretro_core_options.h
{
    "freeintvds_display_layout",
    "Display Layout",
    {
        { "vertical_dual", "Vertical Dual-Screen (Ayaneo/3DS)" },
        { "horizontal_dual", "Horizontal Dual-Screen" },
        { "single_screen", "Single Screen (352x224)" },
        { NULL, NULL },
    },
    "vertical_dual"
}
```

Then in `retro_run()`:
```c
if (layout_option == VERTICAL_DUAL) {
    // Use 1024x1486 (current)
} else if (layout_option == HORIZONTAL_DUAL) {
    // Use 1488x512 (game left, overlay right)
} else {
    // Use 352x224 (single screen)
}
```

### Resolution Recommendations for 3DS

| Layout | Dimension | Aspect | Notes |
|--------|-----------|--------|-------|
| Vertical Dual | 1024×1486 | 1:1.45 | Current (good for 3DS) |
| Horizontal Dual | 1488×512 | 2.9:1 | Alternative (might not fit) |
| Single Screen | 352×224 | 1.57:1 | Fallback |

**Best for 3DS**: **1024×1486 vertical** (already implemented!)

---

## What CAN We Use From Libretro Library

### 1. Core Options System (libretro_core_options.h)
```c
// Already have this!
// Add more sophisticated options:
{
    "freeintvds_3ds_layout",
    "3DS Layout Mode",
    {
        { "auto", "Auto (Detect Device)" },
        { "vertical", "Vertical (Top Game, Bottom Overlay)" },
        { "single", "Single Screen Game Only" },
        { NULL, NULL },
    },
    "auto"
}
```

### 2. Environment Query: Get Target Refresh Rate
```c
#define RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE (50 | RETRO_ENVIRONMENT_EXPERIMENTAL)
```

Can be used to optimize for 3DS refresh rate (59.8 Hz)

### 3. Software Framebuffer API (When Performance Matters)
```c
// Consider for future optimization
if (Environ(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &buffer_info)) {
    // Render directly to frontend's buffer (no copy)
}
```

### 4. Hardware Render Context (GPU-Accelerated Path)
```c
// If 3DS frontend supports GPU rendering
if (Environ(RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER, &hw_context)) {
    // Could use GPU for scaling/compositing
}
```

---

## Comparison: FreeIntvDS vs MelonDS Architecture

### MelonDS Standalone (True Dual-Screen)
- Two separate GPU/CPU context switches
- Separate framebuffers per screen
- Custom rendering pipeline outside libretro

### MelonDS Libretro (Single Framebuffer)
- Renders both screens into one texture
- Uses vertical or horizontal packing
- `retro_video_refresh()` sends combined buffer
- Frontend handles screen routing

### FreeIntvDS (Current Implementation)
- ✅ Renders game + overlay into single 1024x1486 texture
- ✅ Frontend automatically maps to dual screens
- ✅ Works with 3DS, Ayaneo, MelonDS, etc.
- ✅ No core API changes needed

---

## 3DS-Specific Considerations

### Performance Impact
3DS is **much slower** than Ayaneo:
- CPU: ARM11 @ 268 MHz (vs Ayaneo Ryzen ~2.5 GHz)
- Memory: 128 MB (vs Ayaneo 8GB)
- GPU: 320×240 maximum

### Potential Issues at 1024×1486
⚠️ **Scaling down to 3DS res** (320×240 per screen):
- 1024×1486 → Scale 3.2x vertically, 3.2x horizontally
- Still integer scaling (good!)
- May need to verify framerate on actual hardware

### Fallback Options
If 1024×1486 causes performance problems:
1. **512×744** (0.5x scale) - Still dual-screen, half resolution
2. **352×224** (single-screen) - Proven working baseline

---

## Recommended Next Steps

### 1. Verify 3DS RetroArch Support (Quick Check)
```bash
# Check if 3DS RetroArch exposes RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE
# This tells us what refresh rate 3DS expects (59.8 Hz for 3DS)
```

### 2. Add Framerate Detection (Optional Optimization)
```c
retro_usec_t target_fps = 0;
if (Environ(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_fps)) {
    // Adjust emulation speed to 3DS refresh rate
    // Currently hardcoded to DefaultFPS (59.92)
}
```

### 3. Test Current Build on 3DS
- Deploy `libretro_freeintvds.so` to 3DS
- Load Intellivision ROM
- Check if:
  - ✅ Core loads
  - ✅ Displays on both screens
  - ✅ Audio plays
  - ✅ Input works
  - ⚠️ Framerate acceptable (30+ FPS)

### 4. If Performance Issues Arise
Try 512×744 intermediate resolution:
```c
#define WORKSPACE_WIDTH 512
#define WORKSPACE_HEIGHT 744
#define GAME_SCREEN_HEIGHT 326
#define OVERLAY_HEIGHT 450
```

---

## Code Changes Needed for 3DS Optimization

### Option 1: No Changes Required
Current 1024×1486 will work on 3DS as-is.

### Option 2: Add Layout Detection (Recommended)
```c
// In retro_init()
if (Environ(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_fps)) {
    // We're on 3DS (59.8 Hz)
    // Could adjust resolution if needed
    is_3ds_target = true;
}
```

### Option 3: Add Core Options for User Control
```c
// In check_variables()
if (strcmp(var.key, "freeintvds_layout") == 0) {
    if (strcmp(var.value, "single") == 0) {
        dual_screen_enabled = 0;
    } else {
        dual_screen_enabled = 1;
    }
}
```

---

## Summary

| Aspect | Status | Recommendation |
|--------|--------|-----------------|
| **Dual-screen libretro support** | ❌ Not available | Use vertical packing (current approach) |
| **Single framebuffer API** | ✅ Available | Already using |
| **Core options system** | ✅ Available | Can enhance for 3DS-specific settings |
| **Hardware render context** | ✅ Available (experimental) | Not needed for 3DS (software rendering) |
| **Current 1024×1486 on 3DS** | ✅ Should work | Test to verify performance |
| **Optimization opportunities** | ✅ Multiple | Target refresh rate, framebuffer API |

**Bottom Line**: Libretro doesn't offer dual-screen magic, but FreeIntvDS is already correctly implemented for 3DS via vertical framebuffer packing. The 3DS frontend will handle routing pixels to the right display.

---

## Related Files

- `src/libretro.c` - Current implementation
- `src/deps/libretro-common/include/libretro.h` - Complete API reference
- `libretro_core_options.h` - Options system (can be enhanced)
- `ANDROID_SCALED_DUAL_SCREEN.md` - Vertical scaling approach (same for 3DS)
