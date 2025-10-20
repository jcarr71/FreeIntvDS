# Android Core Diagnostic Guide

## Issue: Core Not Loading

If the FreeIntvDS core is recognized by RetroArch but doesn't run when you load a game, use this guide to diagnose.

## Diagnostic Steps

### Step 1: Capture Full Logcat During Core Load

**Before running game:**
```powershell
# Clear logcat
adb logcat -c

# Start capturing (in background)
adb logcat > freeintv_debug.log
```

**Then:**
1. In RetroArch, load a ROM (ZIP or .rom)
2. Select cores → choose "FreeIntvDS"
3. Game attempts to load...

**After ~10 seconds, stop logcat:**
```powershell
# (Stop the logcat command with Ctrl+C)
```

**Examine the log:**
```powershell
Get-Content freeintv_debug.log | Select-String "FREEINTV|error|Error|ERROR|core|Core"
```

### Step 2: Check for These Key Messages

#### ✅ Expected Success Messages
```
[INFO] [FREEINTV] Primary system directory: /sdcard/RetroArch/system
[INFO] Found exec.bin at: /sdcard/RetroArch/system/exec.bin
[INFO] Found grom.bin at: /sdcard/RetroArch/system/grom.bin
[DEBUG] [FREEINTV] Loading exec from: /sdcard/RetroArch/system/exec.bin
[DEBUG] [FREEINTV] Loading grom from: /sdcard/RetroArch/system/grom.bin
```

#### ❌ Expected Error Messages (Indicates Problem)
```
[ERROR] [FREEINTV] SystemPath is NULL!
→ RetroArch not passing system directory path

[ERROR] [FREEINTV] Could not find exec.bin in any location
→ BIOS file missing from all known locations

[ERROR] [FREEINTV] Could not find grom.bin in any location
→ GROM file missing

[WARNING] [FREEINTV] Could not find [FILE] in any known location
→ Searched these paths: (check which ones exist)
```

### Step 3: Verify BIOS Files Manually

```powershell
# Check BIOS file existence and size on device
adb shell ls -lh /sdcard/RetroArch/system/exec.bin
adb shell ls -lh /sdcard/RetroArch/system/grom.bin

# Verify MD5 checksums
adb shell md5sum /sdcard/RetroArch/system/exec.bin
# Should match: 62e761035cb657903761800f4437b8af

adb shell md5sum /sdcard/RetroArch/system/grom.bin
# Should match: 0cd5946c6473e42e8e4c2137785e427f
```

### Step 4: Check Core Library Structure

```powershell
# Verify core is installed
adb shell ls -lh /sdcard/RetroArch/cores/libretro_freeintvds.so

# Verify .info file
adb shell ls -lh /sdcard/RetroArch/info/freeintvds_libretro.info

# Verify core can be read by libretro
adb shell file /sdcard/RetroArch/cores/libretro_freeintvds.so
# Should indicate: ELF shared object
```

## Multi-Path BIOS Search Order

The core now searches for BIOS files in this order:

1. **Primary** (RetroArch passes): `/sdcard/RetroArch/system/`
2. **Fallback 1**: `/sdcard/RetroArch/system/` (if custom install)
3. **Fallback 2**: `/sdcard/Android/data/com.retroarch/files/system/`
4. **Fallback 3**: `/storage/emulated/0/RetroArch/system/`
5. **Fallback 4**: `/storage/emulated/0/Android/data/com.retroarch/files/system/`

**Interpretation**: If logcat shows "Found exec.bin at: [PATH]", the core successfully located it in one of these paths.

## Common Issues & Fixes

### Issue: "SystemPath is NULL"
**Cause**: RetroArch configuration issue or core initialization timing  
**Fix**:
```powershell
# Restart RetroArch completely
adb shell am force-stop com.retroarch
adb shell am start -n com.retroarch/.MainActivity
```

### Issue: "Could not find exec.bin in any location"
**Cause**: BIOS file doesn't exist in any known directory  
**Fix**:
```powershell
# Copy exec.bin to primary location
adb push "path\to\exec.bin" "/sdcard/RetroArch/system/"

# Verify it arrived
adb shell ls -lh /sdcard/RetroArch/system/exec.bin
```

### Issue: "Could not find grom.bin in any location"
**Cause**: GROM file missing  
**Fix**:
```powershell
# Copy grom.bin
adb push "path\to\grom.bin" "/sdcard/RetroArch/system/"

# Verify
adb shell ls -lh /sdcard/RetroArch/system/grom.bin
```

### Issue: Core Loads But Display is Blank/Black
**Possible Causes**:
- Overlay loading failed (check for overlay PNG files)
- STIC video chip initialization issue
- Aspect ratio mismatch in RetroArch

**Debug**:
```powershell
# Check for overlay errors in logcat
adb logcat | Select-String "OVERLAY|CONTROLLER"

# List overlay directory
adb shell ls -lh /sdcard/RetroArch/system/freeintvds-overlays/
```

### Issue: Video Rendering Issues (Static/Corruption)
**Cause**: PNG overlay pixel format conversion issue  
**Debug**:
```powershell
# Capture pixel conversion debug output
adb logcat | Select-String "pixel|format|ARGB|BGR"

# Verify PNG files are valid
adb shell file /sdcard/RetroArch/system/freeintvds-overlays/*.png
```

## Getting Detailed Logs

### Full Initialization Trace

Uncomment these debug flags in `jni/Android.mk`:
```makefile
# Add to LOCAL_CFLAGS:
-DDEBUG_ANDROID -DDEBUG_PNG_LOADING -DDEBUG_PIXEL_FORMAT
```

Then rebuild:
```powershell
cd c:\Users\Jason\Documents\Visual_Studio\FreeIntvDS
ndk-build NDK_PROJECT_PATH=. clean
ndk-build NDK_PROJECT_PATH=.
adb push libs\arm64-v8a\libretro_freeintvds.so /sdcard/RetroArch/cores/
```

### Capture and Save Logcat

```powershell
# Real-time: tail last 100 lines
adb logcat | Select-String "FREEINTV|error|Error" | tail -100

# Save to file for analysis
adb logcat > freeintv_$(Get-Date -Format 'yyyyMMdd_HHmmss').log

# Filter important lines
Get-Content freeintv_debug.log | Where-Object {
    $_ -match "FREEINTV|ERROR|WARN|Found|Looking|Loading"
} | Out-File freeintv_filtered.log
```

## Next Steps if Still Not Working

1. **Verify architecture**: 
   - Most devices: `arm64-v8a` 
   - Use `adb shell getprop ro.product.cpu.abi` to check

2. **Try different architecture** (if device supports):
   ```powershell
   adb push libs\armeabi-v7a\libretro_freeintvds.so /sdcard/RetroArch/cores/
   ```

3. **Clear RetroArch cache**:
   - RetroArch Menu → Settings → Directories
   - Note down directories
   - Restart RetroArch
   - Retry

4. **Rebuild core with additional debugging**:
   - Edit `jni/Android.mk`
   - Add more `-D` flags
   - Rebuild and redeploy

---

**Debug Date**: October 19, 2025  
**Core Version**: FreeIntvDS (cross-platform paths fixed)  
**Last Verified**: Android with armv8 architecture
