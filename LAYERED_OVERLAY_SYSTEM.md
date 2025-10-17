# Layered Overlay System - Final Implementation

**Date:** October 17, 2025  
**Status:** ✅ Complete and Working

## Overview

FreeIntvDS now features a layered dual-screen overlay system designed for the Ayn Thor Android handheld device. The system displays a 2x scaled game screen on top with a layered controller overlay below.

## Display Configuration

### Total Workspace: 704x1068 pixels

**Top Section (Game Screen):** 704x448 pixels
- Source: 352x224 original game resolution
- Scaling: 2x pixel doubling (maintains 1.571:1 aspect ratio)
- Position: Top of screen

**Bottom Section (Overlay Region):** 704x620 pixels
- Left area (x: 0-257): Black background (258 pixels)
- Right area (x: 258-703): Layered overlay system (446 pixels)

## Layered Overlay System

### Layer 1 (Back): Game-Specific Overlay
- **Dimensions:** 370x600 pixels
- **Position:** Centered horizontally within controller base region
- **Horizontal offset:** 38 pixels from controller base left edge
- **Absolute position:** x: 296-665 (370 pixels wide)
- **Vertical alignment:** Top-aligned
- **Rendering:** 1:1 pixel mapping (no scaling)

### Layer 2 (Front): Static Controller Base
- **Dimensions:** 446x620 pixels
- **Position:** Right-aligned to screen edge
- **Absolute position:** x: 258-703 (446 pixels wide)
- **Rendering:** 1:1 pixel mapping (no scaling)
- **Transparency:** Alpha channel preserved, game overlay shows through transparent areas

## File Locations

### Controller Base (Static)
- **Primary:** `system/freeintvds-overlays/controller_base.png`
- **Fallback:** `system/freeintvds-overlays/default.png`
- **Required dimensions:** 446x620 pixels
- **Format:** PNG with alpha transparency
- **Purpose:** Numeric keypad template that remains visible across all games

### Game Overlays (Dynamic)
- **Path:** `system/freeintvds-overlays/[ROM_NAME].png`
- **Fallback:** `system/freeintvds-overlays/default.png`
- **Required dimensions:** 370x600 pixels
- **Format:** PNG with alpha transparency
- **Purpose:** Game-specific overlays that appear centered under the controller base

### Fallback Behavior
- If no game-specific overlay found, uses `default.png`
- If no files found, generates 4-quadrant color test pattern (370x600)
- Controller base and game overlay use same fallback system independently

## Technical Implementation

### Image Loading
```c
// Controller base: Loaded at native 446x620 resolution
controller_base_width = width;   // 446
controller_base_height = height; // 620
// Buffer allocated: width * height * sizeof(unsigned int)

// Game overlay: Loaded at native 370x600 resolution
overlay_width = width;   // 370
overlay_height = height; // 600
// Buffer allocated: width * height * sizeof(unsigned int)
```

### Centering Calculation
```c
// Controller base right-aligned
ctrl_base_x_start = WORKSPACE_WIDTH - controller_base_width;  // 704 - 446 = 258

// Game overlay centered within controller base
game_overlay_x_offset = (controller_base_width - overlay_width) / 2;
// (446 - 370) / 2 = 38 pixels
```

### Rendering Order
1. **Game screen** renders to top 704x448 region (2x scaled)
2. **Game overlay** (back layer) renders centered at native size
3. **Controller base** (front layer) renders on top with transparency
4. Alpha blending allows game overlay to show through controller base transparent areas

### Pixel Format
- **Storage:** ARGB format (0xAARRGGBB)
- **Alpha handling:** Full alpha channel support
- **Color order:** Blue in high byte, Red in low byte for BGR display compatibility

## Build Information

### Compilation
```powershell
mingw32-make          # Builds freeintvds_libretro.dll
```

### Core Identification
- **Core name:** FreeIntvDS (when `FREEINTV_DS` defined)
- **Output file:** `freeintvds_libretro.dll` (Windows)

## Code Structure

### Key Files Modified
- `src/libretro.c`: Main dual-screen and overlay implementation

### Key Functions
```c
load_controller_base()      // Loads 446x620 controller base PNG
load_overlay_for_rom()      // Loads 370x600 game overlay PNG
render_dual_screen()        // Composites all layers into final 704x1068 output
```

### Key Variables
```c
// Workspace dimensions
WORKSPACE_WIDTH = 704
WORKSPACE_HEIGHT = 1068
GAME_SCREEN_HEIGHT = 448
OVERLAY_HEIGHT = 620

// Controller base
controller_base_width = 446
controller_base_height = 620
controller_base (buffer)

// Game overlay
overlay_width = 370
overlay_height = 600
overlay_buffer (buffer)
```

## Cleanup Performed

### Removed Unused Code
- ✅ `OVERLAY_WIDTH` constant (replaced with dynamic `overlay_width`)
- ✅ `button_windows` array (replaced with PNG-based system)
- ✅ Forward declaration for `render_button_labels()`
- ✅ Stray closing brace from removed structures

### Remaining (Harmless)
- Legacy button rendering functions (unused but kept for potential future use)
- Minor compiler warnings about unused functions

## Testing Checklist

- [x] Controller base loads at 446x620 without scaling
- [x] Game overlay loads at 370x600 without scaling
- [x] Game overlay centers horizontally within controller base (38px offset each side)
- [x] Both layers use 1:1 pixel mapping
- [x] Controller base aligns to right edge of screen (x: 258-703)
- [x] Game overlay top-aligns vertically
- [x] Alpha transparency works correctly
- [x] Fallback to default.png works
- [x] Test pattern generates correctly when no files found

## Aspect Ratio

```c
// Calculated as workspace dimensions
aspect_ratio = (float)WORKSPACE_WIDTH / (float)WORKSPACE_HEIGHT;
// 704 / 1068 = 0.659 ≈ 2:3 portrait orientation
```

This matches the Ayn Thor bottom screen portrait mode.

## Next Steps (Future Development)

1. **Android NDK build** - Compile for Android ARM architecture
2. **Touch input mapping** - Map touch coordinates to Intellivision keypad buttons
3. **RetroArch configuration** - Set up aspect ratio and display settings
4. **Overlay creation** - Design game-specific 370x600 overlay PNGs
5. **Controller base design** - Create 446x620 numeric keypad PNG template

## Notes

- All images maintain native aspect ratios (no stretching or distortion)
- Controller base can have transparent windows for game overlay to show through
- System automatically handles different ROM naming formats
- Works with both PNG and JPG formats (PNG preferred for transparency)
- Memory is efficiently managed with dynamic allocation based on actual image sizes
