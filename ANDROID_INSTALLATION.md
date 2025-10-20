# FreeIntvDS Android Installation Guide

## Prerequisites
- RetroArch Android app installed on your device
- Intellivision BIOS files (`exec.bin`, `grom.bin`)
- Intellivision ROM files (`.int`, `.bin`, or `.rom`)

## Installation Steps

### 1. Locate RetroArch Cores Directory
The Android core files are located in RetroArch's cores directory:
```
/sdcard/Android/data/com.retroarch/files/cores/
```

Or depending on your Android version and RetroArch configuration:
```
/data/data/com.retroarch/files/cores/
```

### 2. Copy Core Libraries by Architecture
Copy the compiled `.so` files to the appropriate architecture-specific folders:

**For ARM64 devices (most modern devices):**
```
libs/arm64-v8a/libretro_freeintvds.so → 
  /sdcard/Android/data/com.retroarch/files/cores/libretro_freeintvds_arm64-v8a.so
```

**For 32-bit ARM:**
```
libs/armeabi-v7a/libretro_freeintvds.so → 
  /sdcard/Android/data/com.retroarch/files/cores/libretro_freeintvds_armeabi-v7a.so
```

**For x86:**
```
libs/x86/libretro_freeintvds.so → 
  /sdcard/Android/data/com.retroarch/files/cores/libretro_freeintvds_x86.so
```

**For x86-64:**
```
libs/x86_64/libretro_freeintvds.so → 
  /sdcard/Android/data/com.retroarch/files/cores/libretro_freeintvds_x86_64.so
```

### 3. Copy .info File
Copy the core info file to the cores directory:
```
freeintvds_libretro.info → 
  /sdcard/Android/data/com.retroarch/files/cores/freeintvds_libretro.info
```

### 4. Place BIOS Files
Copy required BIOS files to RetroArch's system directory:
```
exec.bin, grom.bin → 
  /sdcard/Android/data/com.retroarch/files/system/
```

**MD5 Hashes (for verification):**
- `exec.bin`: `62e761035cb657903761800f4437b8af`
- `grom.bin`: `0cd5946c6473e42e8e4c2137785e427f`

### 5. Optional: Create Overlay Directory
For custom controller overlays (advanced):
```
/sdcard/Android/data/com.retroarch/files/system/freeintvds-overlays/
```

Place PNG overlay files here named to match your ROM files:
- `[ROM_NAME].png` - Game-specific overlay (370x600 pixels)
- `controller_base.png` - Default controller template (446x620 pixels)
- `default.png` - Fallback overlay

### 6. Load Content
1. Open RetroArch on your device
2. Go to **Main Menu** → **Load Content**
3. Navigate to your Intellivision ROM file
4. RetroArch should now recognize and list **FreeIntvDS** as an available core
5. Select it and the game should launch

## Troubleshooting

### Core Not Recognized
- **Problem**: RetroArch doesn't show FreeIntvDS as an option
- **Solution**: 
  1. Verify the `.so` file is named `libretro_freeintvds_[ARCH].so` (exact name match)
  2. Ensure `freeintvds_libretro.info` is in the cores directory
  3. Restart RetroArch

### "BIOS Files Not Found"
- **Problem**: Game loads but shows error about missing exec.bin/grom.bin
- **Solution**:
  1. Verify BIOS files are in `/system/` (not `/cores/`)
  2. Check filenames are exactly `exec.bin` and `grom.bin` (lowercase)
  3. Verify MD5 hashes match the expected values
  4. Copy files again if needed

### Game Freezes on Load
- **Problem**: Game starts loading but never gets past splash screen
- **Solution**:
  1. Try with a different ROM to isolate the issue
  2. Check device storage space (need at least 50MB free)
  3. Verify BIOS files are valid (not corrupted)
  4. Check logcat for errors: `adb logcat | grep "RetroArch\|FreeIntv"`

### Display Issues
- **Problem**: Game displays incorrectly (wrong resolution, stretched, etc.)
- **Solution**:
  1. Go to **Settings** → **Video** → **Aspect Ratio** and set to `Core`
  2. Check **Resolution** is not being forced in settings

## File Structure Reference
```
/sdcard/Android/data/com.retroarch/files/
├── cores/
│   ├── libretro_freeintvds_arm64-v8a.so    (Required for ARM64)
│   ├── libretro_freeintvds_armeabi-v7a.so  (Optional, for ARM32)
│   ├── libretro_freeintvds_x86.so          (Optional, for x86)
│   ├── libretro_freeintvds_x86_64.so       (Optional, for x86-64)
│   └── freeintvds_libretro.info            (Required)
├── system/
│   ├── exec.bin                            (Required BIOS)
│   ├── grom.bin                            (Required BIOS)
│   └── freeintvds-overlays/
│       ├── default.png                     (Optional)
│       ├── controller_base.png             (Optional)
│       └── [ROM_NAME].png                  (Optional per-game overlays)
└── downloads/
    └── [Your Intellivision ROMs]
```

## Performance Notes
- **ARM64-v8a**: Best performance (recommended)
- **ARMv7**: Good performance on older devices
- **x86/x86-64**: Slowest, mainly for emulated Android environments

## Building Locally
To rebuild the Android core:
```bash
ndk-build NDK_PROJECT_PATH=.
```

The compiled libraries will be in `libs/[ABI]/libretro_freeintvds.so`

## For Ayaneo Pocket DS Users
The Ayaneo Pocket DS has two screens, making it ideal for FreeIntvDS:
- **Top screen (1600x1080)**: Displays 704x1068 vertical layout
- **Bottom screen**: Can be configured for touch input (future)

The core is optimized for this form factor with proper scaling and overlay support.
