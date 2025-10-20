# Android Core Testing - Dual-Screen Disabled

## Build Info

**Date**: October 19, 2025  
**Version**: FreeIntvDS (Dual-Screen DISABLED - Single Screen Only)  
**Architecture**: ARM64-v8a (3.89 KB)

## What Changed

The dual-screen rendering has been **disabled** to test if the 704x1068 resolution was causing the Android crash.

**Change made**:
```c
// Line 146 in src/libretro.c
static int dual_screen_enabled = 0;  // Was: 1
```

## What to Expect

- **Display resolution**: 352x224 (game screen only)
- **No overlays**: Just the game display
- **Simpler rendering**: Less memory usage, fewer operations

## Deploy Instructions

### Step 1: Push Core to Android Device

```powershell
# For ARM64 (Ayaneo Pocket DS)
adb push "libs\arm64-v8a\libretro_freeintvds.so" "/sdcard/RetroArch/cores/"

# Verify it copied
adb shell ls -lh /sdcard/RetroArch/cores/libretro_freeintvds.so
```

### Step 2: Test in RetroArch

1. **Restart RetroArch** completely (close and reopen)
2. **Load an Intellivision ROM** (ZIP or .rom file)
3. **Select FreeIntvDS core** from the cores menu
4. **Game should start** 

### Step 3: Monitor for Success/Failure

**If game loads and plays:**
```
✅ SUCCESS - Core works without dual-screen
→ Problem is the 704x1068 resolution
→ Need to use side-by-side layout (608x224) or horizontal split
```

**If core still crashes:**
```
❌ FAIL - Problem is not the display resolution
→ Issue is PNG loading, memory allocation, or initialization
→ Try Mode 2: Disable PNG overlays
```

## Debugging Output

Check logcat for any errors:

```powershell
adb logcat | Select-String "FREEINTV|error|Error|ERROR|Loaded|core"
```

Expected output:
```
[INFO] [FREEINTV] Primary system directory: /sdcard/RetroArch/system
[INFO] Found exec.bin at: /sdcard/RetroArch/system/exec.bin
[INFO] Found grom.bin at: /sdcard/RetroArch/system/grom.bin
[DEBUG] [FREEINTV] Loading exec from: ...
[DEBUG] [FREEINTV] Loading grom from: ...
```

## Next Steps Based on Result

### If Test Passes ✅

The issue is the **704x1068 vertical display layout**. Solutions:

1. **Use side-by-side layout**: 352x224 game + 256x224 overlay = 608x224 total
2. **Use horizontal split**: 352x224 game + 352x224 overlay = 704x224 total
3. **Implement true dual-screen** (separate framebuffers for top/bottom screens)

### If Test Still Fails ❌

Try Mode 2: Disable PNG overlay loading
- Edit `retro_load_game()` and comment out `load_overlay_for_rom()`
- Rebuild and test

### Build Log Reference

```
ARM64 Build: ✓ Success
File: libretro_freeintvds.so
Size: 3.89 KB (arm64-v8a)
Status: Ready for Android deployment
```

---

**Status**: Ready to deploy to Android device  
**Test Mode**: Single-Screen (Dual-Screen Disabled)  
**Next Action**: Push to device and test ROM loading
