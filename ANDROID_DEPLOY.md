# Quick Deploy Guide - FreeIntvDS Android Core

## Latest Build (October 19, 2025)

**Status**: ✅ Fixed - Cross-platform path handling

**Issue Fixed**: Windows path separators prevented core loading on Android

## Deploy Steps

### Step 1: Push Core to Device
```powershell
# For ARM64 (most common on modern Android devices)
adb push "libs\arm64-v8a\libretro_freeintvds.so" "/sdcard/RetroArch/cores/"

# For 32-bit (if needed)
adb push "libs\armeabi-v7a\libretro_freeintvds.so" "/sdcard/RetroArch/cores/"

# For Intel x86/x64 (if device uses Intel CPU)
adb push "libs\x86\libretro_freeintvds.so" "/sdcard/RetroArch/cores/"
adb push "libs\x86_64\libretro_freeintvds.so" "/sdcard/RetroArch/cores/"
```

### Step 2: Verify Core Appears in RetroArch
1. Open RetroArch on Android device
2. Load any Intellivision ROM (ZIP or ROM file)
3. Press Menu (or assigned button)
4. Navigate to: **Information → Cores**
5. Look for: **FreeIntvDS** in the list

### Step 3: Select and Test
1. Load a ROM
2. Press Menu → **Load Core**
3. Select **FreeIntvDS**
4. Game should start loading...

### Step 4: Monitor Startup (Optional)
```powershell
# In PowerShell, monitor debug output:
adb logcat | Select-String "FREEINTV|RetroArch|Loaded"
```

Expected output:
```
[INFO] [FREEINTV] Primary system directory: /sdcard/RetroArch/system
[INFO] Found exec.bin at: /sdcard/RetroArch/system/exec.bin
[INFO] Found grom.bin at: /sdcard/RetroArch/system/grom.bin
[DEBUG] [FREEINTV] Loading exec from: /sdcard/RetroArch/system/exec.bin
[DEBUG] [FREEINTV] Loading grom from: /sdcard/RetroArch/system/grom.bin
```

## File Requirements

### BIOS Files (Required)
**Location**: `/sdcard/RetroArch/system/`
- `exec.bin` (62.5 KB) - MD5: `62e761035cb657903761800f4437b8af`
- `grom.bin` (4 KB) - MD5: `0cd5946c6473e42e8e4c2137785e427f`

### Optional: Per-Game Overlays
**Location**: `/sdcard/RetroArch/system/freeintvds-overlays/`
- `controller_base.png` (446×620) - Controller base image
- `[ROM_NAME].png` (370×600) - Per-game overlays
- `default.png` (370×600) - Fallback overlay

## Troubleshooting

### Core Not Listed in Cores Menu
- ✅ Verify file copied to: `/sdcard/RetroArch/cores/libretro_freeintvds.so`
- ✅ Verify file name is exactly: `libretro_freeintvds.so` (not `libretro.so`)
- ✅ Verify `.info` file exists: `/sdcard/RetroArch/info/freeintvds_libretro.info`

### Game Starts but Crashes Immediately
- ✅ Verify `exec.bin` exists and has correct MD5
- ✅ Verify `grom.bin` exists and has correct MD5
- ✅ Check logcat for error messages: `adb logcat | Select-String "FREEINTV"`

### Game Doesn't Use Custom Overlays
- ✅ Verify overlay files exist: `/sdcard/RetroArch/system/freeintvds-overlays/`
- ✅ Verify PNG files are named correctly (match ROM name without extension)
- ✅ Use forward slashes in filenames (no backslashes)

### Display Shows Wrong Resolution
- Check RetroArch video settings
- **Expected**: 704×1068 (vertical dual-screen layout)
  - Top: 704×448 (game screen, 2x scaled)
  - Bottom: 704×620 (overlay/keypad region)

## Architecture Selection

| Device Type | Recommended |
|------------|---|
| Modern Ayaneo/Onhand | **arm64-v8a** |
| Generic Android Phone | arm64-v8a (fallback: armeabi-v7a) |
| Emulator on Windows | x86_64 |
| Emulator on Mac | arm64-v8a (Apple Silicon) |

Most modern Android devices support **arm64-v8a**.

---

**Last Updated**: October 19, 2025  
**Build Type**: Fixed cross-platform paths  
**Status**: Ready for testing
