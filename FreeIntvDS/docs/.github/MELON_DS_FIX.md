# melonDS Analysis & PNG Overlay Fix

## Critical Discovery: MelonDS vs FreeIntvDS Difference

### MelonDS Architecture
- **Both screens rendered by same GPU** (Nintendo DS GPU)
- Both framebuffers use **same pixel format**
- Can use direct `memcpy()` for both screens

### FreeIntvDS Architecture  
- **Game screen**: Rendered by STIC emulator in **BGR format** (0x00BBGGRR)
- **Overlay screen**: External PNG image in **RGBA format**
- Need format conversion to match!

## The Real Problem

### STIC Uses BGR Format
```c
// From stic.c color table:
unsigned int colorTable[] = {
    0xFF2D00,   // Blue  - BGR: BB=00, GG=2D, RR=FF
    0x003EFF,   // Red   - BGR: BB=FF, GG=3E, RR=00
    // ...
};
```

### PNG Uses RGBA Format
stb_image returns RGBA (R,G,B,A bytes in memory order)

### Solution: Convert PNG to BGR
Match the STIC's BGR format:

```c
// Convert RGBA → BGR (to match STIC)
dst_row[x] = 0xFF000000 | (b << 16) | (g << 8) | r;
```

## Changes Made

### 1. Force RGBA Format from stb_image
```c
img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
```

### 2. Convert RGBA → BGR (Not RGB!)
```c
for (int y = 0; y < copy_height; y++) {
    unsigned char* src_row = img_data + src_y * width * 4;
    uint32_t* dst_row = overlay_buffer + y * OVERLAY_WIDTH;
    
    for (int x = 0; x < copy_width; x++) {
        unsigned char r = src_row[x * 4 + 0];
        unsigned char g = src_row[x * 4 + 1];
        unsigned char b = src_row[x * 4 + 2];
        
        // BGR format to match STIC: 0xXXBBGGRR
        dst_row[x] = 0xFF000000 | (b << 16) | (g << 8) | r;
    }
}
```

## Why Initial RGB Approach Failed

Initial attempt used RGB format (0xXXRRGGBB):
```c
dst_row[x] = 0xFF000000 | (r << 16) | (g << 8) | b;  // Wrong!
```

This caused color mismatch because:
- Game screen: BGR format from STIC
- Overlay screen: RGB format from PNG
- Result: Colors swapped (red↔blue)
