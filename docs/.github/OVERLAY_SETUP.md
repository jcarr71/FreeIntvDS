# FreeIntvDS Overlay Setup Guide

## Overview
FreeIntvDS supports custom controller overlays displayed on the right screen (256x224) while the game displays on the left screen (352x224).

## Quick Setup

### 1. Create Overlay Directory
Create a folder in your RetroArch system directory:
```
<RetroArch>/system/freeintvds-overlays/
```

Example: `C:\RetroArch-Win64\system\freeintvds-overlays\`

### 2. Add Overlay Images
Place overlay images in the folder, named to match your ROM files (without extension):

**Supported formats**: PNG, JPG

**Examples**:
- `Astrosmash (USA, Europe).jpg` - for Astrosmash ROM/ZIP
- `Space Armada (USA).png` - for Space Armada ROM/ZIP
- `default.jpg` - fallback overlay for games without specific overlays

### 3. Image Requirements
- **Recommended size**: Any size (auto-scaled with bilinear interpolation)
- **Aspect ratio**: Preserved during scaling
- **Display area**: 256x224 pixels (portrait images work well)

## File Matching

The core matches overlays by ROM filename (without extension):

| ROM File | Looks For (in order) |
|----------|---------------------|
| `Game.zip` | `Game.png` → `Game.jpg` → `default.png` → `default.jpg` |
| `Game.rom` | `Game.png` → `Game.jpg` → `default.png` → `default.jpg` |
| `Game.bin` | `Game.png` → `Game.jpg` → `default.png` → `default.jpg` |

**Note**: Works with ZIPped ROMs - RetroArch extracts to temp, but the core uses the original ZIP name.

## Example Directory Structure
```
C:\RetroArch-Win64\
  system\
    exec.bin
    grom.bin
    freeintvds-overlays\
      Astrosmash (USA, Europe).jpg
      Space Armada (USA).jpg
      default.jpg
  roms\
    intellivision\
      Astrosmash (USA, Europe).zip
      Space Armada (USA).zip
```

## Overlay Creation Tips

### Intellivision Keypad Layout
```
1   2   3
4   5   6
7   8   9
CLR 0   ENT
```

### Recommended Tools
- GIMP, Photoshop, or Paint.NET for image editing
- Portrait orientation (e.g., 450x700) scales well
- Use high-contrast colors for visibility on small screens

### Color Considerations
- The core uses BGR pixel format internally
- Colors are automatically converted from RGBA (PNG/JPG) to BGR
- No manual color adjustment needed

## Troubleshooting

**Overlay not loading?**
1. Check filename matches ROM exactly (case-sensitive on some systems)
2. Verify file is in `system/freeintvds-overlays/`
3. Ensure dual-screen mode is enabled in core options
4. Try adding a `default.jpg` to test if path detection works

**Overlay looks pixelated?**
- Use higher resolution source images
- Current implementation uses bilinear interpolation for smooth scaling

**Need generic overlay for all games?**
- Create `default.jpg` or `default.png` in the overlays folder
- Will be used for any game without a specific overlay

## Core Options
Check RetroArch core options for:
- **Dual Screen Mode**: Enable/disable dual-screen display
- Make sure it's set to ON for overlays to appear

## Future Enhancements
Planned features:
- Touch input mapping for overlay buttons (Android target)
- Multiple overlay sets (game-specific, generic numeric pad, etc.)
- Overlay configuration per-game
