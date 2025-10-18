# FreeIntvDS Keyboard Keypad Mapping Guide

## Quick Start: What Works Now

✅ **Gamepad/Joystick Support** (RECOMMENDED):

| Button | Action |
|--------|--------|
| **L2** (Left Trigger) | Cycle through hotspots 0-11 |
| **R2** (Right Trigger) | Clear selection (no highlight) |
| **L3** (Left Stick Click) | Highlight Clear button (*) |
| **R3** (Right Stick Click) | Highlight Enter button (#) |

The overlay will highlight in real-time as you press these buttons, showing which keypad button would be activated.

**How to Use:**
1. Load a game in RetroArch with FreeIntvDS
2. Press **L2** repeatedly to cycle through all 12 hotspots and verify positions
3. Press **L3** to highlight the Clear button - verify it's in the right position
4. Press **R3** to highlight the Enter button - verify it's in the right position
5. Press **R2** to clear the highlight

⚠️ **Keyboard Support** (Limited):
- `1 2 3 4 5 6 7 8 9 0` → Number buttons ✅ (WORKING)
- Other keys may be consumed by RetroArch's menu system

**Note:** L3 and R3 (stick clicks) are used for hotspot testing since they don't conflict with game controls.

## Enabling Keyboard Input in RetroArch

RetroArch cores can capture raw keyboard input. FreeIntvDS has this enabled.

| Intellivision Button | Suggested Key | Alternative |
|----------------------|--------------|-------------|
| 1 | `1` (numpad or number row) | `Q` |
| 2 | `2` (numpad or number row) | `W` |
| 3 | `3` (numpad or number row) | `E` |
| 4 | `4` (numpad or number row) | `A` |
| 5 | `5` (numpad or number row) | `S` |
| 6 | `6` (numpad or number row) | `D` |
| 7 | `7` (numpad or number row) | `Z` |
| 8 | `8` (numpad or number row) | `X` |
| 9 | `9` (numpad or number row) | `C` |
| Clear (*) | `[` (left bracket) | `K` |
| 0 | `0` (numpad or number row) | `SPACE` |
| Enter (#) | `]` (right bracket) | `L` |

## Testing Without Configuration

You can start testing immediately with hard-coded keyboard bindings:

```
Number Keys:     1-9, 0 → Buttons 1-9, 0
Clear (*):       [ → Button *
Enter (#):       ] → Button #
```

Just press these keys while FreeIntvDS is running - the overlay buttons will highlight automatically!

## How the Overlay Works

- **Game Screen**: Left side (352×224) shows the actual game
- **Controller Overlay**: Right side (256×224) shows the numeric keypad
- **Button Highlighting**: Buttons highlight in yellow/green when pressed
- **Visual Feedback**: Semi-transparent overlay indicates active button

## Configuration Files

- **System Directory**: RetroArch system directory (typically `~/.config/retroarch/system/` or `C:\Users\[USER]\AppData\Roaming\RetroArch\system\`)
- **Required Files**:
  - `exec.bin` - Intellivision Executive ROM
  - `grom.bin` - Intellivision Graphics ROM

## Troubleshooting

**Overlay buttons don't highlight when I press keys:**
- Ensure the FreeIntvDS core is loaded (`freeintvds_libretro.dll`)
- Check that `exec.bin` and `grom.bin` are in the RetroArch system directory
- Try using a RetroArch gamepad instead of keyboard
- Press keys while the game window has focus

**Keyboard input not being detected:**
- Go to `Settings` → `Input` → `Input Driver` and try different drivers (`sdl2`, `dinput`, etc.)
- Check that you're not pressing keys during the emulation pause screen
- Try the hard-coded keys first (1-9, 0, [, ]) to verify the core is working

**Wrong button highlighted:**
- The overlay hotspot layout matches the Intellivision keypad positions
- Verify hotspot coordinates are correctly calculated
- Check the console output for debug messages

## Development Notes

- The overlay supports custom PNG files placed in `overlays/[ROM_NAME].png` relative to the ROM directory
- Button highlighting is driven by the active Intellivision controller memory at address `0x1FE`
- The system automatically detects pressed buttons and highlights matching hotspots

