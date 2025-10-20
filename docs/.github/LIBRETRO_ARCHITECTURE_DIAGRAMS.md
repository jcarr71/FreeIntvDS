# Libretro Dual-Screen Architecture Diagrams

## 1. Single vs Dual-Screen Rendering

### Traditional Single-Screen Emulator
```
┌─────────────────────────────────┐
│      Game Memory (16KB RAM)      │
├─────────────────────────────────┤
│      CPU Execution (CP1610)      │
├─────────────────────────────────┤
│     Video Output (STIC)          │
│         352×224 BGR              │
├─────────────────────────────────┤
│    Framebuffer (352×224 ARGB)    │
├─────────────────────────────────┤
│  video_cb(buffer, 352, 224, ...)│◄── Libretro
├─────────────────────────────────┤
│  Frontend displays on TV/Screen  │
└─────────────────────────────────┘
```

### FreeIntvDS Dual-Screen Architecture
```
┌──────────────────────────────────────────────────────┐
│            Emulation State (Memory + CPU)             │
├──────────────────────────────────────────────────────┤
│  STIC Video Output (352×224 BGR)  +  PNG Overlay     │
├──────────────────────────────────────────────────────┤
│        Composite Rendering Function                   │
│  ┌────────────────────────────────┐                   │
│  │ 1. 2× Upscale Game → 704×448  │                   │
│  │ 2. Render Overlay → 704×620   │                   │
│  │ 3. Blend in Framebuffer       │                   │
│  └────────────────────────────────┘                   │
├──────────────────────────────────────────────────────┤
│        Combined Framebuffer                           │
│  ┌─────────────────────────────────┐                  │
│  │  704×448 Game (2× Upscaled)    │ ← Top Screen     │
│  ├─────────────────────────────────┤                  │
│  │  704×620 Overlay + Controllers │ ← Bottom Screen  │
│  └─────────────────────────────────┘                  │
│  Total: 1024×1486 ARGB Buffer     │                  │
├──────────────────────────────────────────────────────┤
│ video_cb(buffer, 1024, 1486, ...) ◄── Libretro      │
├──────────────────────────────────────────────────────┤
│              Frontend (RetroArch)                      │
│  ┌──────────────┐  ┌──────────────┐                  │
│  │ Scale top    │  │ Scale bottom │                  │
│  │ portion to   │  │ portion to   │                  │
│  │ 3DS/Ayaneo   │  │ 3DS/Ayaneo   │                  │
│  │ top screen   │  │ bottom screen│                  │
│  └──────────────┘  └──────────────┘                  │
└──────────────────────────────────────────────────────┘
```

---

## 2. Framebuffer Layout Comparison

### Layout 1: Vertical Stacking (FreeIntvDS Current)
```
Combined 1024×1486 Buffer Layout:

┌─────────────────────────────────────┐
│                                     │
│        Game Screen (2× scaled)      │
│            704×448 pixels           │
│     (Upscaled from 352×224)         │
│                                     │
├─────────────────────────────────────┤
│                                     │
│      Overlay / Controller Pad       │
│            704×620 pixels           │
│   (Game-specific + base overlays)   │
│                                     │
└─────────────────────────────────────┘

For 3DS:
  Top 1024×652 (approx) → 3DS top screen (scales 3.2×)
  Bottom 1024×834 (approx) → 3DS bottom screen (scales 3.2×)
```

### Layout 2: Horizontal Side-by-Side (Alternative)
```
Combined 608×224 Buffer Layout:

┌────────────┬────────────────┐
│   Game     │    Overlay     │
│ 352×224px  │   256×224px    │
│            │                │
│  (left)    │    (right)     │
└────────────┴────────────────┘

For Ayaneo top screen (left-right): Better utilization
For 3DS (top-bottom): Wouldn't work as well
```

### Layout 3: Single Screen (Fallback)
```
Simple 352×224 Buffer:

┌──────────────────┐
│                  │
│  Game Screen     │
│    Only          │
│  352×224 pixels  │
│                  │
└──────────────────┘

For 3DS: Uses only top screen, bottom is blank
For Ayaneo: Doesn't use bottom screen capability
```

---

## 3. 3DS Frontend Rendering Pipeline

### What Happens After video_cb()
```
FreeIntvDS Core:
    render_dual_screen()
         ↓
    Video(combined_buffer, 1024, 1486, pitch)
         ↓
    ┌────────────────────────────────────────────┐
    │  RetroArch 3DS Frontend                     │
    │  (libretro port for Nintendo 3DS)          │
    ├────────────────────────────────────────────┤
    │ 1. Receives 1024×1486 ARGB texture         │
    │                                             │
    │ 2. Splits framebuffer:                      │
    │    ├─ Top portion (1024×≈650): Game       │
    │    └─ Bottom portion (≈1024×836): Overlay │
    │                                             │
    │ 3. Scales for 3DS hardware:                │
    │    ├─ Top: 1024×≈650 → 400×240 (3DS top) │
    │    └─ Bot: 1024×≈836 → 320×240 (3DS bot) │
    │                                             │
    │ 4. Renders to dual screens                 │
    │    ├─ Top LCD: 400×240 (upper screen)     │
    │    └─ Bot LCD: 320×240 (lower screen)     │
    └────────────────────────────────────────────┘
         ↓
    3DS Dual-Screen Display
    [400×240 Game]  ← Top Screen
    [320×240 Overlay] ← Bottom Screen
```

---

## 4. Libretro API Call Sequence

### Per-Frame Execution
```
┌─────────────────────────────────────────────┐
│  retro_run()  [Called ~60 times per second] │
├─────────────────────────────────────────────┤
│                                             │
│  1. Poll Input                              │
│     └─ InputPoll()                          │
│     └─ InputState(port, device, ...)        │
│                                             │
│  2. Execute Emulation                       │
│     └─ exec() loop (CPU, STIC, PSG, etc)   │
│                                             │
│  3. Composite Frames                        │
│     └─ render_dual_screen()                 │
│        ├─ Scale game 2×                     │
│        ├─ Render overlay                    │
│        └─ Alpha blend                       │
│                                             │
│  4. Send Output to Frontend                 │
│     └─ Video(buffer, 1024, 1486, pitch) ◄──┐
│                                          │  │
│     └─ audio_cb(left, right) ◄──────────┼──┐ (Also audio)
│                                          │  │
└─────────────────────────────────────────────┘  │
     ↓                                          │
    Frontend (RetroArch) receives data ←───────┘
     ↓
    Frontend renders to display hardware
```

---

## 5. Pixel Format and Stride

### Pixel Layout in Memory
```
Each Pixel: ARGB (32-bit)
┌────────────────────────┐
│ Alpha (8-bit)          │
│ Red (8-bit)            │
│ Green (8-bit)          │
│ Blue (8-bit)           │
└────────────────────────┘

For 1024×1486 Buffer:
  Width: 1024 pixels
  Height: 1486 pixels
  Pitch: 1024 × 4 bytes = 4096 bytes per scanline

Total Size: 1024 × 1486 × 4 = 6,087,680 bytes (~5.8 MB)
```

### Memory Layout (Linear Buffer)
```
Memory Address:                 Pixel Position:
     ↓                               ↓
  Buffer[0]        →  Pixel (0, 0) - Top-left
  Buffer[1]        →  Pixel (1, 0)
  Buffer[2]        →  Pixel (2, 0)
  ...
  Buffer[1023]     →  Pixel (1023, 0) - Top-right
  Buffer[1024]     →  Pixel (0, 1) - Next scanline
  Buffer[1025]     →  Pixel (1, 1)
  ...
  Buffer[4096]     →  Pixel (0, 1) - Start of scanline 1 (if stride=4096)
  
Game area (top):    Buffer[0] → Buffer[704×448×4-1]
Overlay area (bot): Buffer[704×448×4] → Buffer[704×1486×4-1]
```

---

## 6. Layout Decision Tree for 3DS

```
3DS RetroArch loads FreeIntvDS
            ↓
    What's the current layout?
            ↓
    ┌─────────────────────────────────────┐
    │   Check environment variable        │
    │   (if implemented in core)          │
    └─────────────────────────────────────┘
            ↓
    ┌──────────────────────────────────┐
    │ Current: Vertical (1024×1486)    │
    │ - Renders fine                   │
    │ - Both screens used              │
    │ - ~40 FPS typical                │
    └──────────────────────────────────┘
            ↓
    If user wants to change (optional feature):
            ↓
    User selects "Horizontal (608×224)"
            ↓
    Core calls: apply_layout_change(LAYOUT_HORIZONTAL)
            ↓
    Core sends: Environ(SET_GEOMETRY, new_geometry)
            ↓
    Frontend reconfigures display
            ↓
    If performance issues:
            ↓
    User selects "Single Screen (352×224)"
            ↓
    Core uses fallback 352×224 layout
```

---

## 7. Comparison: FreeIntvDS vs MelonDS on 3DS

### MelonDS (Nintendo DS Emulator)
```
DS Screens: 256×192 each

MelonDS Buffer:
    ┌─────────────────────────┐
    │  Top DS Screen          │
    │    256×192              │
    ├─────────────────────────┤
    │  Bottom DS Screen       │
    │    256×192              │
    └─────────────────────────┘
    Total: 512×384

To 3DS:
    Scale 512×384 → scales differently per screen
    OR render at 3DS-native resolution (400×240 + 320×240)
```

### FreeIntvDS (Intellivision + Overlays)
```
Game: 352×224 → 2× upscale → 704×448
Overlay: 704×620 (fixed size)

FreeIntvDS Buffer:
    ┌──────────────────────────┐
    │  Game (2× upscaled)      │
    │      704×448             │
    ├──────────────────────────┤
    │  Overlay (fixed size)    │
    │      704×620             │
    └──────────────────────────┘
    Total: 1024×1486 (after padding/scaling)

To 3DS:
    Scale 1024×1486 to fit both screens
    (Automatic by RetroArch)
```

### Key Difference
- **MelonDS**: Two real screens from original hardware
- **FreeIntvDS**: One screen + UI overlay (custom arrangement)

Both use single framebuffer. Both work on 3DS via same mechanism.

---

## 8. Resolution Scaling Math

### For 3DS (Two 400×240 screens, or 320×240 bottom)
```
FreeIntvDS Output: 1024×1486
3DS Combined: ~720×480 (max)

Scaling Factor:
  720 ÷ 1024 = 0.703 (width compression)
  480 ÷ 1486 = 0.323 (height compression)

Result: Image will be squashed slightly vertically
        (Acceptable - maintains playability)

Better: Use 704×1068 or 512×744 to match 3DS aspect better
```

### For Ayaneo Pocket DS (Top ~1600×1080, Bottom ~1280×720)
```
FreeIntvDS Output: 1024×1486
Ayaneo Combined Effective: ~1600×1800 (if both screens used)

Scaling Factor:
  1024 ÷ 1600 = 0.64 (upscale to fill width)
  1486 ÷ 1800 = 0.826 (upscale to fill height)

Result: Good fit, scales up cleanly (integer or nearly integer)
```

---

## 9. Buffer Composition Code Flow

```
render_dual_screen() in src/libretro.c:

┌─────────────────────────────────────────┐
│ render_dual_screen()                     │
├─────────────────────────────────────────┤
│                                          │
│ ┌─ Step 1: Get STIC output              │
│ │  STIC.updateDisplay()                 │
│ │  Returns: 352×224 BGR pixels          │
│ │           (in temporary buffer)       │
│ │                                        │
│ ├─ Step 2: 2× Upscale to 704×448       │
│ │  scale_2x_blit()                      │
│ │  Input:  352×224 BGR                  │
│ │  Output: 704×448 ARGB at offset 0    │
│ │          in frame[] buffer            │
│ │                                        │
│ ├─ Step 3: Load overlay PNGs            │
│ │  load_overlay_for_rom()               │
│ │  Returns: Pointer to PNG pixels       │
│ │           (if PNG found)              │
│ │                                        │
│ ├─ Step 4: Render overlay to buffer    │
│ │  draw_overlay()                       │
│ │  Input:  PNG texture (704×620)       │
│ │  Output: Blended at offset 704×448   │
│ │          in frame[] buffer            │
│ │          with alpha compositing      │
│ │                                        │
│ └─ Step 5: Send to libretro            │
│    Video(frame, 1024, 1486, 1024*4)   │
│                                          │
└─────────────────────────────────────────┘
     ↓
Frontend receives combined texture
```

---

## 10. Summary: Architecture at a Glance

```
                    3DS RetroArch
                         ↑
                    video_cb()
                   (1024×1486)
                         ↑
                  ┌──────────────┐
                  │ FreeIntvDS   │
                  │   Core       │
                  ├──────────────┤
                  │ Composite    │
                  │ Game + UI    │
                  ├──────────────┤
                  │ Game Render  │
                  │ 2× upscale   │
                  ├──────────────┤
                  │ PNG Loading  │
                  │ Alpha Blend  │
                  ├──────────────┤
                  │ Emulation    │
                  │ (CP1610 etc) │
                  └──────────────┘
                         ↑
            Intellivision Game ROM

         Everything flows correctly! ✓
```

