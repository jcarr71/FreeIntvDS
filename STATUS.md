# FreeIntvDS - Current Status
**Last Updated**: October 17, 2025

## ✅ Completed Features

### Dual-Screen Display (Vertical Layout for Ayn Thor)
- **Total Resolution**: 704x1068 pixels
- **Game Screen**: 704x448 (top section, 2x scaled from 352x224)
- **Overlay Region**: 704x620 (bottom section, layered)
- **Rendering**: Vertical stacked layout in single framebuffer
- **Performance**: Efficient single-pass rendering with 1:1 pixel mapping
- **Toggle**: Core option to enable/disable dual-screen mode

### Layered Overlay System
- **Controller Base**: 446x620 pixels, right-aligned static template
- **Game Overlay**: 370x600 pixels, centered under controller base
- **Location**: `<RetroArch>/system/freeintvds-overlays/`
- **Format support**: PNG with alpha transparency (JPG fallback)
- **Naming**: 
  - Controller base: `controller_base.png`
  - Game overlays: `[ROM_NAME].png`
- **Fallback**: `default.png` for both controller base and game overlays
- **ZIP support**: Works with archived ROMs (uses original ZIP name)
- **Layering**: Game overlay renders underneath controller base with alpha blending

### Image Processing
- **Scaling**: None - all images use 1:1 pixel mapping (native resolution)
- **Aspect ratio**: Perfectly preserved (no stretching or distortion)
- **Centering**: Game overlay automatically centered within 446px controller base
- **Alignment**: 
  - Game overlay: Top-aligned, horizontally centered
  - Controller base: Top-aligned, right-aligned to screen edge
- **Color format**: ARGB with full alpha channel support
- **Transparency**: Alpha blending allows overlay to show through controller base

### Build System
- **Windows**: MinGW-w64 (`mingw32-make`)
- **Core identification**: "FreeIntvDS" when `FREEINTV_DS` defined
- **Clean compilation**: No errors, minimal warnings
- **Output**: `freeintvds_libretro.dll`

## 🔄 Working Implementation Details

### Display Layout
```
┌─────────────────────────────────┐
│    Game Screen (704x448)        │  ← 2x scaled, centered
│    2x pixel doubling            │
├─────────────────────────────────┤
│ [258px black] [446px overlay]   │  ← Controller region
│               ┌───────────┐     │
│               │ Ctrl Base │     │  ← 446x620, right-aligned
│               │ 446x620   │     │
│               │  ┌─────┐  │     │
│               │  │ Ovr │  │     │  ← Game overlay 370x600
│               │  │370x │  │     │     centered, underneath
│               │  │600  │  │     │
│               │  └─────┘  │     │
│               └───────────┘     │
└─────────────────────────────────┘
  Total: 704x1068 pixels
```

### Path Detection
- Stores RetroArch system directory at init
- Builds overlay paths relative to system dir
- Handles temp extraction (RetroArch unzips to temp)
- Filename matching works across platforms
- Separate loading for controller base and game overlays

### Rendering Pipeline
1. **Game frame** rendered by STIC (352x224, BGR format)
2. **2x upscaling** to 704x448 using pixel doubling
3. **Game copied** to top section (0-447 vertical)
4. **Controller base** loaded at 446x620 native resolution
5. **Game overlay** loaded at 370x600 native resolution
6. **Layer compositing**:
   - Black background (left 258 pixels)
   - Game overlay rendered (centered, back layer)
   - Controller base rendered on top (front layer, with alpha)
7. **Single texture** (704x1068) sent to RetroArch frontend

### Memory Management
- **Controller base buffer**: Allocated at actual image dimensions (446x620)
- **Overlay buffer**: Allocated at actual image dimensions (370x600)
- **Dual-screen buffer**: Allocated once at 704x1068, reused
- **Image data**: Freed immediately after loading
- **Clean shutdown**: All buffers freed in `retro_deinit()`
- **Dynamic reallocation**: Buffers freed and reallocated if ROM changes

## 📋 Testing Status

### Tested Platforms
- ✅ Windows 10/11 with RetroArch
- ✅ ZIP extraction handling
- ✅ PNG loading
- ✅ JPG loading
- ✅ Bilinear scaling quality
- ✅ Fallback to default overlay

### Pending Testing
- ⏳ Android build (NDK)
- ⏳ Ayaneo Pocket DS hardware
- ⏳ Touch input (Android target)
- ⏳ Performance on mobile hardware

## 🎯 Next Steps

### Android Deployment (Priority)
1. Build with Android NDK (jni/ folder ready)
2. Test on Ayaneo Pocket DS
3. Verify dual-screen display on hardware
4. Test overlay visibility on small screens

### Touch Input (Future)
1. Map touch zones to Intellivision keypad
2. Enable overlay buttons to be interactive
3. Configure per-game touch layouts

### Enhancements (Nice-to-have)
1. Multiple overlay sets per game
2. User-configurable overlay opacity
3. Overlay selection menu
4. Automatic overlay downloads

## 📝 Code Quality

### Production Ready
- ✅ All debug code removed
- ✅ Clean function naming (`render_dual_screen`, `dual_screen_buffer`)
- ✅ No hardcoded paths
- ✅ Proper error handling
- ✅ Memory safety (bounds checking, null checks)

### Documentation
- ✅ Overlay setup guide (OVERLAY_SETUP.md)
- ✅ Updated README.md
- ✅ Code comments maintained
- ✅ Build instructions clear

## 🐛 Known Issues
- None currently reported

## 💾 Development Environment

### Windows Build
- Compiler: MinGW-w64 GCC
- Make: `mingw32-make`
- IDE: Visual Studio Code
- Output: `freeintvds_libretro.dll`

### File Structure
```
src/
  libretro.c         - Main dual-screen implementation
  intv.c             - Emulator core
  cp1610.c           - CPU
  stic.c             - Video (generates 352x224 game frame)
  psg.c              - Audio
  ivoice.c           - Voice synthesis
  stb_image.h        - Image loading library (v2.30+)
```

### Key Constants
```c
GAME_WIDTH = 352
GAME_HEIGHT = 224
OVERLAY_WIDTH = 256
DUAL_WIDTH = 608  (352 + 256)
DUAL_HEIGHT = 224
```

## 🔗 Repository
- GitHub: jcarr71/FreeIntvDS
- Branch: master
- Based on: libretro/FreeIntv

## 📧 Contact
Ready for Android deployment and hardware testing.
