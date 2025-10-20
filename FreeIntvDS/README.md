# FreeIntvDS
FreeIntvDS is a libretro core for the Intellivision console with **dual-screen support**, designed for dual-screen handheld devices like the Ayaneo Pocket DS.

Based on FreeIntv by David Richardson, with enhancements by Oscar Toledo G. and Joe Zbiciak.

## Features
- **Dual-screen display**: Game on left (352x224), controller overlay on right (256x224)
- **Custom overlays**: Load game-specific or default controller images from system directory
- **Bilinear scaling**: Smooth overlay rendering with interpolation
- **ZIP support**: Works with archived ROMs (overlays matched by ROM name)
- Standard Intellivision emulation (CPU, PSG, STIC, Intellivoice)

## Quick Start

### BIOS Requirements
Place these files in RetroArch's **system** directory:

| Function | Filename* | MD5 Hash |
| --- | --- | --- | 
| Executive ROM | `exec.bin`  | `62e761035cb657903761800f4437b8af` |
| Graphics ROM | `grom.bin` | `0cd5946c6473e42e8e4c2137785e427f` |

* BIOS filenames are case-sensitive

### Overlay Setup
1. Create folder: `<RetroArch>/system/freeintvds-overlays/`
2. Add controller overlay images (PNG or JPG) named to match your ROM files
3. Optional: Add `default.jpg` as fallback for games without specific overlays

**See [OVERLAY_SETUP.md](OVERLAY_SETUP.md) for detailed instructions.**

## License
FreeIntvDS is licensed under GPLv2+. Based on FreeIntv by David Richardson.

## Entertainment Computer System
FreeIntv does not currently support Entertainment Computer System (ECS) functionality. Contributions to the code are welcome!

## Controller overlays
Mattel Intellivision games were often meant to be played with game-specific cards overlaid on the numeric keypad. These overlays convey information which can be very useful in gameplay. Images of a limited selection of Intellivision titles are available at: http://www.intellivisionlives.com/bluesky/games/instructions.shtml

## Controls

* **Mini-Keypad** - allows the user to view and select keys from a small Intellivision pad in the lower corner of the display.
* **Controller Swap** - Some Intellivision games expect the left controller to be player one, others expect the right controller. This isn't a problem if you have two controllers (and don't mind juggling them) but users with only one controller or using a portable setup would be effectively locked out of some games. Controller Swap swaps the two controller interfaces so that the player does not have to physically swap controllers.

| RetroPad | FreeIntv Function |
| --- | --- |
| D-Pad| 8-way movement |
| Left Analog Stick | 16-way disc |
| Right Analog Stick | 8-way keypad |
| L3 | Keypad 0 |
| R3 | Keypad 5 |
| L2 | Keypad Clear |
| R2 | Keypad Enter |
| A | Left Action Button |
| B | Right Action Button |
| Y | Top Action Button |
| X | Use the Last Selected Intellivision Keypad Button. In Astrosmash, for example, you can leave "3" selected to enable instant access to hyperspace. |
| L/R | Activate the Mini-Keypad |
| Start | Pause Game |
| Select | Controller Swap |
