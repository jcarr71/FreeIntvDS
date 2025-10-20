# Intellivision Controller Keypad Code Reference

## Keypad Button Codes (from `controller.c`)

The Intellivision keypad has 12 buttons arranged in a 4×3 grid:

```
1   2   3
4   5   6
7   8   9
C   0   E
```

### Button Hex Codes

| Button | Hex Code | Binary | Variable |
|--------|----------|--------|----------|
| 1 | 0x81 | 10000001 | K_1 |
| 2 | 0x41 | 01000001 | K_2 |
| 3 | 0x21 | 00100001 | K_3 |
| 4 | 0x82 | 10000010 | K_4 |
| 5 | 0x42 | 01000010 | K_5 |
| 6 | 0x22 | 00100010 | K_6 |
| 7 | 0x84 | 10000100 | K_7 |
| 8 | 0x44 | 01000100 | K_8 |
| 9 | 0x24 | 00100100 | K_9 |
| 0 (Zero) | 0x48 | 01001000 | K_0 |
| C (Clear) | 0x88 | 10001000 | K_C |
| E (Enter) | 0x28 | 00101000 | K_E |

**Note**: These codes represent the bit patterns sent to the Intellivision processor on the PSG (Programmable Sound Generator) control lines.

## Keypad State Array

```c
int keypadStates[12] = { 
    K_1, K_2, K_3,     // Row 0 (index 0-2)
    K_4, K_5, K_6,     // Row 1 (index 3-5)
    K_7, K_8, K_9,     // Row 2 (index 6-8)
    K_C, K_0, K_E      // Row 3 (index 9-11)
};
```

**Array Indexing**: `keypadStates[(cursorY * 3) + cursorX]`
- `cursorY` = row (0-3)
- `cursorX` = column (0-2)
- Example: Button "5" (middle) = index (1*3)+1 = 4

## Keypad Reading in FreeIntvWorkingKeypad

### How the Core Reads Keypad Input

1. **File**: `src/libretro.c`
2. **Memory Location**: `Memory[0x1FE]` (Right Controller / Player 0)
3. **Current Implementation**:
   ```c
   int current_key = Memory[0x1FE] ^ 0xFF;  // XOR with 0xFF to invert logic
   
   // Check if key is pressed by ANDing with keypad code
   if ((current_key & overlay_hotspots[i].keypad_code) == overlay_hotspots[i].keypad_code) {
       // Button is pressed - highlight hotspot
   }
   ```

### Hotspot Mapping (FreeIntvWorkingKeypad)

The hotspots in `libretro.c` are initialized with the same keypad codes:

```c
int keypad_map[12] = { 
    K_1(0x02), K_2(0x03), K_3(0x04),
    K_4(0x13), K_5(0x23), K_6(0x14),
    K_7(0x84), K_8(0x44), K_9(0x24),
    K_C(0x88), K_0(0x48), K_E(0x28)
};
```

**Important Note**: These are DIFFERENT from controller.c codes! 
- `controller.c` uses: 0x81, 0x41, 0x21, etc. (actual hardware codes)
- `libretro.c` uses: 0x02, 0x03, 0x04, etc. (abstracted bit patterns)

## Keypad Control in Original FreeIntv

### `drawMiniKeypad()` Function
Draws a small on-screen keypad display with cursor highlighting:
- Renders 27×39 pixel keypad image
- Draws green cursor box around selected button
- Updates position based on `cursor[player]` variables

### `getQuickKeypadState()` Function
Returns the keypad button code for the currently selected cursor position:
```c
int getQuickKeypadState(int player) {
    int cursorX = cursor[player*2];
    int cursorY = cursor[player*2+1];
    return keypadStates[(cursorY*3) + cursorX];
}
```

## Your Hotspot Implementation

In `FreeIntvWorkingKeypad/src/libretro.c`, the hotspots:
- Store keypad codes in `overlay_hotspots[i].keypad_code`
- Read button presses from `Memory[0x1FE]`
- Highlight active hotspots with semi-transparent green overlay
- Display wireframe grid with button labels (0-9, C, E)

## Integration Notes

1. **Button Press Detection**: Uses bit-wise AND operation on keypad code
2. **Visual Feedback**: Hotspots highlight when corresponding button is pressed
3. **Cursor Position**: Maps cursor X,Y coordinates to keypad button index
4. **Control Mapping**: PSG bit patterns match Intellivision keypad layout

## References

- **Controller Source**: `FreeIntvWorkingKeypad/src/controller.c` (lines 24-35)
- **Keypad Array**: `FreeIntvWorkingKeypad/src/controller.c` (line 66)
- **Hotspot System**: `FreeIntvWorkingKeypad/src/libretro.c` (lines 78-90, 145-188)
- **Button Detection**: `FreeIntvWorkingKeypad/src/libretro.c` (lines 575-581)
