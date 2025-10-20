# FreeIntvDS Overlay System Guide

## 🎮 Layered Controller Overlay System

FreeIntvDS uses a **3-layer overlay system** that mimics real Intellivision controller overlays:

### Layer Architecture

**Layer 1 (Bottom)**: Game-specific button graphics  
**Layer 2 (Middle)**: Physical controller photo with transparent button windows  
**Layer 3 (Top)**: Button labels (1-9, 0, CLR, ENT)  

This creates an authentic look where game overlays show through the controller's button windows, just like the real physical controller!

---

## 📁 File Structure

### Required Files

**Controller Base Image** (Optional but recommended):
- **Location**: `[RetroArch System Directory]/controller_base.png`
- **Size**: Any size (will be scaled to 256×224)
- **Format**: PNG with alpha channel (RGBA)
- **Content**: Photo of Intellivision controller with **transparent button windows**

**Game-Specific Overlays**:
- **Location**: `[ROM Directory]/overlays/[ROM_NAME].png`
- **Size**: Any size (will be scaled to 256×224)
- **Format**: PNG or JPG
- **Content**: Button graphics specific to the game (e.g., Bomb Squad bomb images)

---

## 🎨 Creating Controller Base Image

### Step 1: Prepare Controller Photo

Using the provided Intellivision controller photo:

1. **Crop the image** to remove:
   - Disc controller (top/bottom)
   - Side action buttons
   - Keep only: Keypad section + surrounding controller body

2. **Make button areas transparent**:
   - Use image editor (Photoshop, GIMP, etc.)
   - Select the 12 button areas
   - Delete to make transparent (alpha = 0)
   - Keep controller body opaque (alpha = 255)

### Step 2: Export Settings

- **Format**: PNG (required for alpha channel)
- **Dimensions**: Recommend 256×224 or larger (will be scaled)
- **Alpha Channel**: Enabled
- **Button Windows**: 12 transparent rectangles for keypad buttons

### Step 3: Installation

Save as: `controller_base.png`  
Place in: RetroArch system directory (same folder as `exec.bin` and `grom.bin`)

---

## 🎯 Button Window Positions

The 12 button windows should be arranged in a 3×4 grid:

```
Row 1:  [1]  [2]  [3]
Row 2:  [4]  [5]  [6]
Row 3:  [7]  [8]  [9]
Row 4: [CLR] [0] [ENT]
```

**Current Default Positions** (can be adjusted):
- Button size: 65×45 pixels each
- Spacing: ~10 pixels between buttons
- Total area: ~256×224 pixels

---

## 🖼️ Creating Game Overlays

### Example: Bomb Squad

Using the provided Bomb Squad overlay image:

1. **Use original overlay as-is** or extract button section
2. **Save as PNG/JPG**: `overlays/Bomb Squad.png`
3. **Dimensions**: Any size (core will scale to fit)
4. **Content**: Just the button graphics (bombs, tools, etc.)

The controller base will composite on top, so only button areas show through!

---

## 📋 File Examples

### RetroArch System Directory
```
C:\RetroArch\system\
├── exec.bin                    # Required: Intellivision BIOS
├── grom.bin                    # Required: Intellivision BIOS
└── controller_base.png         # Optional: Physical controller image
```

### Game ROM Directory
```
C:\Games\Intellivision\
├── Bomb Squad.rom
└── overlays/
    └── Bomb Squad.png          # Game-specific button graphics
```

---

## 🎨 Rendering Pipeline

1. **Load game overlay** from `overlays/[ROM_NAME].png`
2. **Scale to 256×224** (maintains aspect ratio)
3. **Load controller base** from `controller_base.png` (once at startup)
4. **Composite layers**:
   - Draw game overlay (bottom layer)
   - Alpha blend controller base on top (buttons show through transparent windows)
   - Draw button labels (text overlay for clarity)

---

## 💡 Design Tips

### For Controller Base Image:

✅ **Do**:
- Use high-contrast controller photo
- Make button windows **fully transparent** (alpha = 0)
- Keep controller body **opaque** (alpha = 255)
- Include natural shadows/depth for realism
- Crop tight around keypad section

❌ **Don't**:
- Include disc controller or side buttons
- Make entire image transparent
- Use JPG format (no alpha channel)
- Leave button areas opaque

### For Game Overlays:

✅ **Do**:
- Design for button-size areas (65×45 pixels each)
- Use clear, high-contrast graphics
- Match original Intellivision overlay style
- Include visual cues for button function

❌ **Don't**:
- Include controller outline (that's in base image)
- Worry about exact positioning (scaling handles it)
- Use tiny text (will be hard to read)

---

## 🔧 Fallback Behavior

**If controller_base.png not found**:
- System uses game overlay only
- Button labels still render on top
- Still fully functional, just less authentic look

**If game overlay not found**:
- System shows checkerboard test pattern
- Button labels still render
- User knows overlay is missing

---

## 🎯 Example Setup

### Quick Start

1. **Get controller photo** (provided in Discord)
2. **Edit in GIMP/Photoshop**:
   - Crop to keypad section
   - Make 12 button windows transparent
   - Export as `controller_base.png`
3. **Place in RetroArch system directory**
4. **Create game overlays** in `overlays/` subfolder
5. **Launch game** - overlays automatically load!

---

## 📐 Technical Specifications

- **Overlay Display Area**: 256×224 pixels (right screen)
- **Game Display Area**: 352×224 pixels (left screen)
- **Combined Output**: 608×224 pixels (dual-screen)
- **Supported Formats**: PNG (with alpha), JPG
- **Color Format**: ARGB (8-bit per channel)
- **Alpha Blending**: Full support for transparency

---

## 🚀 Next Features

- **Button press highlights**: Light up buttons when pressed
- **Input state mapping**: Track which buttons are active
- **Multiple controller styles**: Support different controller base images
- **Per-game button windows**: Custom button positions per game

---

*Created: October 15, 2025*  
*FreeIntvDS Dual-Screen Intellivision Emulator*
