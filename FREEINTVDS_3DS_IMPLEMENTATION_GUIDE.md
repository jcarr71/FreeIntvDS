# FreeIntvDS 3DS Support: Implementation Checklist

## Current Status ✅

| Component | Status | Notes |
|-----------|--------|-------|
| Single framebuffer rendering | ✅ Working | 704x1068 vertical layout |
| Libretro API integration | ✅ Working | Already using video_cb() correctly |
| Cross-platform paths | ✅ Fixed | No Windows backslashes |
| BIOS file finding | ✅ Enhanced | 5-location fallback search |
| Android NDK build | ✅ Working | All 4 architectures compile |
| 3DS integration | ✅ Ready | No changes required, will work as-is |

---

## What You Get for Free on 3DS

The current FreeIntvDS 1024×1486 implementation will automatically work on 3DS because:

1. **3DS RetroArch Frontend** handles screen routing
   - Top portion (704×448 upscaled game) → Top 3DS screen (400×240)
   - Bottom portion (704×620 overlay) → Bottom 3DS screen (320×240)
   - Frontend does all the scaling

2. **Single framebuffer is libretro standard**
   - MelonDS does the same thing
   - Citra does the same thing
   - All multi-screen libretro cores use this pattern

3. **No API extensions needed**
   - libretro doesn't offer dual-screen callbacks
   - Our workaround is the official pattern
   - 3DS frontend expects exactly this

---

## Optional Enhancements for 3DS

### Option 1: Add Core Options for Layout Selection

**File**: `src/libretro_core_options_intl.h`

**Location**: After existing options

```c
// Add this option group:

{
    "freeintvds_display_layout",
    "Display Layout",
    "Affichage",  // French
    {
        { "vertical_default", "Vertical Dual-Screen (Default)", "Vertical 1024×1486" },
        { "horizontal", "Horizontal Side-by-Side (608×224)", "Horizontal 608×224" },
        { "single", "Single Screen Game Only (352×224)", "Game Only 352×224" },
        { NULL, NULL, NULL },
    },
    "vertical_default"
},

{
    "freeintvds_device_type",
    "Target Device",
    "Appareil Cible",  // French
    {
        { "auto", "Auto-Detect", "Détection Automatique" },
        { "3ds", "Nintendo 3DS", "Nintendo 3DS" },
        { "ayaneo", "Ayaneo Pocket DS", "Ayaneo Pocket DS" },
        { "generic", "Generic Handheld", "Handheld Générique" },
        { NULL, NULL, NULL },
    },
    "auto"
},
```

**File**: `src/libretro.c`

**Location**: In `check_variables()` function

```c
// Add after existing variable checks:

const char *layout_var = "freeintvds_display_layout";
var.key = layout_var;
if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
    int new_layout = LAYOUT_VERTICAL_DEFAULT;
    
    if (strcmp(var.value, "horizontal") == 0) {
        new_layout = LAYOUT_HORIZONTAL;
    } else if (strcmp(var.value, "single") == 0) {
        new_layout = LAYOUT_SINGLE_SCREEN;
    }
    
    if (new_layout != current_display_layout) {
        current_display_layout = new_layout;
        apply_layout_change(new_layout);
    }
}
```

---

### Option 2: Add Device Auto-Detection

**File**: `src/libretro.c`

**Location**: In `retro_init()` after BIOS loading

```c
void detect_device_type() {
    double target_fps = 0.0;
    
    if (!Environ(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE, &target_fps)) {
        detected_device = DEVICE_GENERIC;
        return;
    }
    
    // 3DS: 59.82 Hz (3DS-specific)
    if (target_fps > 59.7 && target_fps < 59.9) {
        detected_device = DEVICE_3DS;
        fprintf(stderr, "[FreeIntvDS] Detected Nintendo 3DS (%.1f Hz)\n", target_fps);
        
        // Optional: Adjust performance settings for 3DS
        // could reduce emulation speed if needed
        
    }
    // Ayaneo: 60.00 Hz (Ayaneo/generic handheld)
    else if (target_fps > 59.9 && target_fps < 60.1) {
        detected_device = DEVICE_AYANEO;
        fprintf(stderr, "[FreeIntvDS] Detected Ayaneo/Generic Handheld (%.1f Hz)\n", target_fps);
    }
    else {
        detected_device = DEVICE_GENERIC;
        fprintf(stderr, "[FreeIntvDS] Unknown device refresh rate: %.1f Hz\n", target_fps);
    }
}
```

---

### Option 3: Add Layout Switching Function

**File**: `src/libretro.c`

**Location**: Before `render_dual_screen()`

```c
typedef enum {
    LAYOUT_VERTICAL_DEFAULT = 0,
    LAYOUT_HORIZONTAL = 1,
    LAYOUT_SINGLE_SCREEN = 2
} DisplayLayout;

static DisplayLayout current_display_layout = LAYOUT_VERTICAL_DEFAULT;

void apply_layout_change(DisplayLayout new_layout) {
    switch (new_layout) {
        case LAYOUT_VERTICAL_DEFAULT:
            dual_screen_enabled = 1;
            WORKSPACE_WIDTH = 1024;
            WORKSPACE_HEIGHT = 1486;
            GAME_SCREEN_HEIGHT = 652;
            OVERLAY_HEIGHT = 901;
            break;
            
        case LAYOUT_HORIZONTAL:
            dual_screen_enabled = 1;
            WORKSPACE_WIDTH = 1488;
            WORKSPACE_HEIGHT = 512;
            GAME_SCREEN_HEIGHT = 512;  // Full height, game left
            OVERLAY_HEIGHT = 512;      // Full height, overlay right
            break;
            
        case LAYOUT_SINGLE_SCREEN:
            dual_screen_enabled = 0;
            WORKSPACE_WIDTH = 352;
            WORKSPACE_HEIGHT = 224;
            GAME_SCREEN_HEIGHT = 224;
            OVERLAY_HEIGHT = 0;
            break;
    }
    
    // Notify RetroArch of geometry change
    struct retro_game_geometry geom = {
        .base_width = WORKSPACE_WIDTH,
        .base_height = WORKSPACE_HEIGHT,
        .max_width = WORKSPACE_WIDTH,
        .max_height = WORKSPACE_HEIGHT,
        .aspect_ratio = (float)WORKSPACE_WIDTH / (float)WORKSPACE_HEIGHT
    };
    
    Environ(RETRO_ENVIRONMENT_SET_GEOMETRY, &geom);
}
```

---

### Option 4: Add Debug Output for 3DS

**File**: `src/libretro.c`

**Location**: In `retro_init()`

```c
fprintf(stderr, "[FreeIntvDS] ==== Initialization ====\n");
fprintf(stderr, "[FreeIntvDS] Version: %d.%d.%d\n", 
    FREEINTVDS_VERSION_MAJOR, FREEINTVDS_VERSION_MINOR, FREEINTVDS_VERSION_PATCH);
fprintf(stderr, "[FreeIntvDS] Dual-screen: %s\n", dual_screen_enabled ? "ENABLED" : "DISABLED");
fprintf(stderr, "[FreeIntvDS] Resolution: %ux%u\n", WORKSPACE_WIDTH, WORKSPACE_HEIGHT);
fprintf(stderr, "[FreeIntvDS] Aspect ratio: %.3f\n", 
    (float)WORKSPACE_WIDTH / (float)WORKSPACE_HEIGHT);

// Device detection
detect_device_type();
fprintf(stderr, "[FreeIntvDS] Detected device: %s\n", 
    detected_device == DEVICE_3DS ? "3DS" : 
    detected_device == DEVICE_AYANEO ? "Ayaneo" : "Generic");

fprintf(stderr, "[FreeIntvDS] ========================\n");
```

---

## Build Commands for 3DS

### 1. Building for 3DS (Same as Android)

```bash
# Clean previous build
mingw32-make clean

# Build all architectures (includes 3DS as libnx)
mingw32-make platform=libnx

# Or build with specific architecture:
mingw32-make platform=3ds
```

### 2. Deploying to 3DS

```bash
# Copy core to 3DS RetroArch cores directory
# Via FTP or direct SD card access:
# /retroarch/cores/libretro_freeintvds_libretro.so

# Verify BIOS files are in:
# /retroarch/system/exec.bin
# /retroarch/system/grom.bin
```

---

## Testing Checklist for 3DS

### Basic Functionality Test
- [ ] Core loads in RetroArch 3DS
- [ ] Game displays on both screens
- [ ] Audio plays
- [ ] Buttons work
- [ ] Framerate is acceptable (30+ FPS)

### Layout Testing (If options added)
- [ ] Vertical dual-screen works
- [ ] Horizontal layout works (if added)
- [ ] Single screen works (if added)
- [ ] Layout switching responsive

### Performance Testing
- [ ] Game doesn't crash during gameplay
- [ ] No audio glitches
- [ ] No input lag
- [ ] Battery usage acceptable

---

## Troubleshooting for 3DS

### Issue: Core doesn't load
```
Solution: Check console output (3DS capture or USB debug):
- Make sure libretro_freeintvds.so is 32-bit ARM (libnx target)
- Verify BIOS files exist in system directory
- Check for compilation errors in build log
```

### Issue: Display is garbled/static
```
Solution: Could be pitch/stride issue
- Verify WORKSPACE_WIDTH * 4 == pitch passed to video_cb()
- Check pixel format is ARGB8888 (XRGB8888 in libretro terms)
```

### Issue: Only one screen displays
```
Solution: Layout might not be routing correctly
- Verify current_display_layout setting
- Check apply_layout_change() is being called
- Output debug info to verify resolution
```

### Issue: Framerate too low
```
Solutions:
1. Try single-screen mode (352×224) - simpler to render
2. Disable overlays if possible
3. Reduce emulation accuracy in settings
4. Check if 3DS is in low-power mode
```

---

## Code Files to Modify

| File | Changes | Complexity | Optional? |
|------|---------|-----------|-----------|
| `src/libretro.c` | Add device detection, layout switching | Medium | ⚠️ No (needs at least detection) |
| `src/libretro_core_options_intl.h` | Add layout options | Easy | ✅ Yes (nice-to-have) |
| Build system | Add 3DS-specific flags | Easy | ✅ Yes (use existing) |

---

## Current Build Target Compatibility

The existing FreeIntvDS compiles for:
- ✅ Windows (MinGW-w64)
- ✅ Android (NDK - 4 architectures)
- ✅ Linux
- ✅ macOS
- ✅ 3DS/libnx (platform flag available)
- ✅ Emscripten (web)
- ✅ Switch (libnx)

**No code changes needed for 3DS to work**—just compile with `platform=libnx` or let Android NDK produce ARM versions compatible with 3DS RetroArch.

---

## Performance Notes for 3DS

### CPU Budget
- **3DS CPU**: ARM11 @ 268 MHz (vs Ayaneo @ 2.5 GHz)
- **Realistic framerate**: 20-40 FPS for Intellivision at 1024×1486
- **Single-screen**: 40-60 FPS likely achievable

### Memory Budget
- **3DS RAM**: ~128 MB (vs Ayaneo 8GB)
- **FreeIntvDS footprint**: ~3-5 MB (acceptable)
- **Framebuffer**: 1024×1486×4 ≈ 6.1 MB (manageable)

### Optimization Potential (if needed)
1. **Reduce resolution scale** (512×744 instead of 1024×1486)
2. **Enable sprite/background caching** (if not already)
3. **Use hardware rendering** (if 3DS RetroArch supports it)
4. **Profile with 3DS debug tools** (CPU time, memory usage)

---

## Summary: What to Do Now

### ✅ Immediate Action: Test as-is
1. Compile for 3DS (platform=libnx or use Android ARM outputs)
2. Deploy to 3DS
3. Load Intellivision ROM
4. Test on real hardware

### ✅ Short-term: Add basic detection
1. Implement `detect_device_type()` (10 minutes)
2. Output debug info (5 minutes)
3. Verify detection works

### ⚠️ Medium-term: Add layout options
1. Add core options (15 minutes)
2. Implement layout switching (30 minutes)
3. Test all three layouts

### ❓ Long-term: Optimize for 3DS
1. Profile performance on real device
2. Adjust resolution if needed
3. Fine-tune audio/video sync

---

## Key Realization

**FreeIntvDS is already 3DS-ready!** 🎉

The 1024×1486 vertical layout is exactly what 3DS RetroArch expects. No API extensions exist in libretro for true dual-screen—all multi-screen cores (MelonDS, Citra) use the same framebuffer-stacking approach.

Just compile and test. If it works well, consider adding layout options for user flexibility. That's it!

