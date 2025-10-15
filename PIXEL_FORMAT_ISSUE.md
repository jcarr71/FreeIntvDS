# Pixel Format Mismatch Issue

## The Problem
You're combining two different pixel formats in one buffer:

### Game Screen (Left Side - from STIC)
- Format: **BGR** (0x00BBGGRR + alpha/flags)
- Source: `frame[]` array from `stic.c`
- Example: Blue = `0xFF2D00` (BB=00, GG=2D, RR=FF)

### Overlay Screen (Right Side - from PNG)
- Format: **RGB** (0xXXRRGGBB) - XRGB8888
- Source: PNG converted via stb_image
- Example: Blue should be `0xFF0000FF` (RR=00, GG=00, BB=FF)

## Evidence from Code

### STIC Color Table (stic.c line 64-73):
```c
unsigned int colorTable[] = {
    0x05000C,   // Black
    0xFF2D00,   // Blue    - BGR: BB=00, GG=2D, RR=FF
    0x003EFF,   // Red     - BGR: BB=FF, GG=3E, RR=00
    0x64D4C9,   // Tan
    // ... etc
};
```

Notice: Blue is `0xFF2D00` not `0x002DFF` - this is BGR format!

### Your PNG Conversion (libretro.c):
```c
// Converts RGBA → XRGB8888 (RGB format)
dst_row[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
```

This creates RGB format (0xXXRRGGBB).

## Why MelonDS Doesn't Have This Problem

MelonDS renders **both screens from the same GPU** (Nintendo DS GPU renders both top and bottom screens), so both use the same pixel format. They can directly memcpy both framebuffers.

You're combining:
1. **Native emulator output** (STIC in BGR format)
2. **External image** (PNG in RGB format)

## Solutions

### Option 1: Convert PNG to BGR (Match STIC)
When loading PNG, convert to BGR format to match the game screen:
```c
// Convert RGBA → BGR (to match STIC format)
dst_row[x] = 0xFF000000 | (b << 16) | (g << 8) | r;  // Swap R and B
```

### Option 2: Convert STIC to RGB (Match PNG)
Convert game frame from BGR to RGB when copying to dual buffer (more expensive):
```c
// When copying game frame, convert BGR → RGB
for (int y = 0; y < GAME_HEIGHT; y++) {
    for (int x = 0; x < GAME_WIDTH; x++) {
        uint32_t bgr_pixel = frame[y * GAME_WIDTH + x];
        uint32_t b = (bgr_pixel >> 16) & 0xFF;
        uint32_t g = (bgr_pixel >> 8) & 0xFF;
        uint32_t r = bgr_pixel & 0xFF;
        uint32_t rgb_pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
        dual_buffer[y * DUAL_WIDTH + x] = rgb_pixel;
    }
}
```

### Option 3: Tell libretro Frontend (Best)
Check if you're setting the pixel format correctly. The frontend might be expecting a different format.

## Recommended Fix
**Option 1** is simplest - just swap R and B when loading the PNG:
```c
dst_row[x] = 0xFF000000 | (b << 16) | (g << 8) | r;  // BGR to match STIC
```

This way both game and overlay use the same BGR format that the STIC already outputs.
