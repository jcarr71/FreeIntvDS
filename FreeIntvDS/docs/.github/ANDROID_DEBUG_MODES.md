# Android Debug/Test Modes

## Issue: Core Not Loading on Android

If the FreeIntvDS core crashes when loading on Android, try these diagnostic modes:

## Mode 1: Test Without Dual-Screen (Simplest Test)

**Hypothesis**: The 704x1068 resolution is causing issues. Test with just the 352x224 game window.

**To Test**:
1. Edit `src/libretro.c` line 146:
   ```c
   static int dual_screen_enabled = 1;
   ```
   Change to:
   ```c
   static int dual_screen_enabled = 0;
   ```

2. Rebuild:
   ```powershell
   cd c:\Users\Jason\Documents\Visual_Studio\FreeIntvDS
   ndk-build NDK_PROJECT_PATH=. clean
   ndk-build NDK_PROJECT_PATH=.
   ```

3. Deploy and test:
   ```powershell
   adb push libs\arm64-v8a\libretro_freeintvds.so /sdcard/RetroArch/cores/
   ```

4. Try loading a game in RetroArch

**Expected Result if Successful**:
- Game displays at 352x224 (small window)
- Can load and play
- Shows core is fundamentally working

## Mode 2: Test Without Overlays (Single Buffer)

**Hypothesis**: PNG overlay loading is causing crashes.

**To Test**:
1. In `src/libretro.c`, comment out the overlay loading call in `retro_load_game()` (line ~943):
   ```c
   // Temporarily disable overlay loading
   // if (dual_screen_enabled && info && info->path) {
   //     load_overlay_for_rom(info->path);
   // }
   ```

2. Also disable controller base loading in `retro_init()` (line ~911):
   ```c
   // // Load static controller base image once at startup
   // load_controller_base();
   ```

3. Rebuild and test

**Expected Result if Successful**:
- Core loads
- No overlay graphics, just solid color background
- Core doesn't crash

## Mode 3: Test Memory Allocation

**Hypothesis**: Buffer allocation is failing on Android.

**To Test**:
1. Add safety checks to `render_dual_screen()` (line ~475):
   ```c
   if (!dual_screen_buffer) {
       dual_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
       if (!dual_screen_buffer) {
           printf("[ERROR] Failed to allocate dual_screen_buffer (need %d bytes)\n", 
                  WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
           return;  // Skip rendering if allocation failed
       }
   }
   ```

2. Add to frame sending (find `video_cb` call):
   ```c
   if (!dual_screen_buffer) {
       printf("[ERROR] dual_screen_buffer is NULL, skipping frame\n");
       return;
   }
   ```

3. Rebuild and monitor logcat

## Mode 4: Minimum Viable Core (MVC)

**Build a "stub" core that does almost nothing:**

In `retro_run()` (line ~1167), replace almost everything with:

```c
void retro_run(void)
{
    // Do nothing for diagnostic
    static int counter = 0;
    
    if (counter % 60 == 0) {
        printf("[TEST] retro_run called (frame %d)\n", counter);
    }
    counter++;
    
    // Just send a black frame
    memset(frame, 0, frameSize);
    video_cb(frame);
}
```

This tests if the core framework itself works without any emulation.

## Interpreting Results

### If Mode 1 Works (352x224 works, 704x1068 crashes):
- **Problem**: Display resolution too large for device
- **Solution**: Use side-by-side layout (352+256=608 x 224) instead of vertical (704x1068)

### If Mode 2 Works (No overlays works, with overlays crashes):
- **Problem**: PNG loading or alpha blending causes crashes
- **Solution**: Remove PNG overlay system, use simple solid color backgrounds

### If Mode 3 Shows Allocation Failures:
- **Problem**: Device doesn't have enough memory
- **Solution**: Reduce buffer sizes or use smaller resolution

### If Mode 4 Fails:
- **Problem**: Fundamental core loading issue, not emulation-specific
- **Solution**: Check core linkage, symbol exports, initialization order

## Quick Diagnostics Checklist

- [ ] BIOS files exist and have correct MD5
- [ ] Core .so file is valid ELF shared object
- [ ] .info file exists with correct naming
- [ ] RetroArch logcat shows "Loaded core" message
- [ ] No "undefined symbol" errors in logcat
- [ ] Core appears in cores list (can be selected)
- [ ] Game ROM loads after core selection
- [ ] Minimal test (Mode 4) works

## Next Steps Based on Results

**If Mode 1 passes**: Implement side-by-side layout (608x224) instead of vertical
**If Mode 2 passes**: Remove PNG system, use fallback rendering  
**If Mode 3 shows issues**: Profile memory usage on device
**If Mode 4 fails**: Debug symbol exports and core initialization

---

**Testing Date**: October 19, 2025  
**Device**: Ayaneo Pocket DS (assumed)  
**Goal**: Isolate root cause of core not loading
