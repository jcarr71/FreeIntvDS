# FreeIntvProjects - Development Environment Guide

## ⚠️ IMPORTANT: WINDOWS PLATFORM ONLY

This workspace is configured for **Windows 64-bit development only**. All agents should assume:
- **OS**: Windows 10/11 (64-bit)
- **Shell**: PowerShell 5.1
- **Compiler**: MinGW64 (GCC 15.2.0)
- **Build Tool**: mingw32-make

## Do NOT Use Unix/Linux Commands

❌ **AVOID**:
- `make`, `head`, `tail`, `ls`, `rm`, `grep`, `sed`, `awk`
- Forward slashes in paths (`C:/path/to/file`)
- Unix-style commands and tools

✅ **USE INSTEAD**:
- `mingw32-make` (not `make`)
- `Get-Item`, `Remove-Item`, `Select-String` (PowerShell cmdlets)
- Backslashes in paths (`C:\path\to\file`)
- Native Windows command-line tools

## Environment Setup

### MinGW64 Location
```
C:\mingw64\bin\
```

### Required Tools
- `mingw32-make.exe` - Build tool
- `gcc.exe` - C compiler
- Other standard GNU toolchain utilities

### Quick Setup in PowerShell
```powershell
# Add MinGW64 to PATH (do this in every new terminal)
$env:PATH += ";C:\mingw64\bin"

# Verify tools are available
mingw32-make --version
gcc --version
```

## Project Structure

```
FreeIntvProjects/                    # Main repository (GitHub)
├── .git/                            # Git repository
├── .vscode/                         # VS Code workspace settings
├── .devenv                          # Development environment config
├── FreeIntvDS/                      # FreeIntvDS project (vertical layout)
│   └── src/
│       ├── libretro.c              # Main dual-screen implementation
│       ├── stb_image.h
│       ├── stb_image_impl.c
│       └── ... (other source files)
├── FreeIntvWorkingKeypad/          # FreeIntvWorkingKeypad clone (horizontal layout)
│   ├── src/
│   │   ├── libretro.c
│   │   ├── stb_image.h
│   │   ├── stb_image_impl.c
│   │   └── ... (other source files)
│   ├── Makefile
│   ├── Makefile.common
│   └── freeintv_libretro.dll       # Compiled output
└── README.md
```

## Branches

### FreeIntvDS
- **Type**: Vertical dual-screen layout
- **Display**: Game screen on top (704×448), overlay below (704×620)
- **Status**: Feature complete
- **Location**: `FreeIntvDS/` folder

### FreeIntvWorkingKeypad
- **Type**: Horizontal keypad layout
- **Display**: Game screen left (704×448), keypad right (370×600), utilities below
- **Status**: ✅ Compiled and ready for testing
- **Compiled**: `freeintv_libretro.dll` (196 KB)
- **Location**: `FreeIntvWorkingKeypad/` folder

## Building the Project

### Prerequisites
1. MinGW64 installed at `C:\mingw64`
2. Git installed
3. PowerShell (comes with Windows)

### Build FreeIntvWorkingKeypad

```powershell
# 1. Add MinGW64 to PATH
$env:PATH += ";C:\mingw64\bin"

# 2. Navigate to project
cd "c:\Users\Jason\Documents\Visual Studio\FreeIntvProjects\FreeIntvWorkingKeypad"

# 3. Build
mingw32-make platform=win

# 4. Output
# freeintv_libretro.dll will be created in the root directory
```

### Clean Build

```powershell
# Remove object files and DLL
Get-Item -Path src\*.o -ErrorAction SilentlyContinue | Remove-Item -Force
Get-Item -Path src\deps\*\*.o -ErrorAction SilentlyContinue | Remove-Item -Force -Recurse
Get-Item -Path *.dll -ErrorAction SilentlyContinue | Remove-Item -Force

# Rebuild
mingw32-make platform=win
```

## Git Workflow

### Current Branches
```powershell
git branch -a
# Output:
#   FreeIntvDS
# * FreeIntvWorkingKeypad
#   master
#   remotes/origin/FreeIntvDS
#   remotes/origin/FreeIntvWorkingKeypad
#   remotes/origin/HEAD -> origin/master
#   remotes/origin/feature/hotspot-overlay-system
#   remotes/origin/master
```

### Common Operations

```powershell
# Check current branch
git branch

# Switch branch
git checkout FreeIntvDS
git checkout FreeIntvWorkingKeypad

# View status
git status

# Stage and commit
git add -A
git commit -m "description of changes"

# Push to GitHub
git push origin branch-name

# Pull latest
git pull origin branch-name
```

## File Operations in PowerShell

### Instead of Unix Commands

| Unix Command | PowerShell Equivalent | Notes |
|---|---|---|
| `ls` | `Get-Item` or `dir` | List files |
| `rm file` | `Remove-Item file` or `del file` | Delete file |
| `rm -r dir` | `Remove-Item -Recurse dir` | Delete directory |
| `cat file` | `Get-Content file` or `type file` | View file contents |
| `grep pattern file` | `Select-String pattern file` | Search in file |
| `mkdir dir` | `New-Item -ItemType Directory dir` | Create directory |
| `cp file1 file2` | `Copy-Item file1 file2` | Copy file |
| `mv file1 file2` | `Move-Item file1 file2` | Move/rename file |

### Examples

```powershell
# List files in directory
Get-Item "C:\Users\Jason\Documents\Visual Studio\FreeIntvProjects\FreeIntvWorkingKeypad\src\"

# Check if DLL exists
Get-Item "C:\Users\Jason\Documents\Visual Studio\FreeIntvProjects\FreeIntvWorkingKeypad\freeintv_libretro.dll"

# View file contents
Get-Content "C:\path\to\file.txt"

# Search in files
Select-String "pattern" "C:\path\to\file.c"

# Copy a file
Copy-Item "C:\source\file.txt" "C:\destination\file.txt"

# Remove object files
Get-Item -Path src\*.o -ErrorAction SilentlyContinue | Remove-Item -Force
```

## Testing the Build

1. Copy `freeintv_libretro.dll` to your RetroArch cores directory
2. Load an Intellivision ROM in RetroArch
3. The horizontal layout should display with:
   - Game screen on the LEFT
   - Keypad on the RIGHT
   - Utility buttons below the game

## Configuration Files

### `.devenv`
- Development environment documentation
- Platform and compiler information
- Project paths
- Important notes for agents

### `.vscode/settings.json`
- VS Code workspace settings
- Terminal configuration (auto-loads MinGW64 in PATH)
- File encoding and EOL settings
- C language formatting preferences

## Compilation Notes

- **Compiler**: MinGW64 GCC 15.2.0
- **Platform Flag**: `platform=win`
- **Build Output**: `freeintv_libretro.dll`
- **Size**: ~196 KB
- **Warnings**: Existing pre-compilation warnings (non-critical)

## Key Implementation Details

### Display System (libretro.c)

```c
#define WORKSPACE_WIDTH 1100    // Game (704) + Keypad (370) + margin
#define WORKSPACE_HEIGHT 620    // Max height needed
#define GAME_SCREEN_WIDTH 704   // 352 * 2x scaling
#define GAME_SCREEN_HEIGHT 448  // 224 * 2x scaling
#define KEYPAD_WIDTH 370        // Overlay width
#define KEYPAD_HEIGHT 600       // Overlay height
```

### Hotspot System
- 12 hotspots (4 rows × 3 columns for Intellivision keypad)
- Positioned on RIGHT side in keypad region
- Semi-transparent highlighting on button press
- Real-time visual feedback

### Utility Buttons
- 6 buttons: MENU, PAUSE, REWIND, SAVE, LOAD, SWAP
- Horizontal layout below game screen
- Y position: 460 pixels
- X positions: 10, 80, 150, 220, 290, 360

## Troubleshooting

### Build Fails: "mingw32-make not found"
- Solution: Add `C:\mingw64\bin` to PATH
- `$env:PATH += ";C:\mingw64\bin"`

### Build Fails: Multiple definition errors
- Solution: Ensure `stb_image_impl.c` is in SOURCES_C list only once
- Check that `STB_IMAGE_IMPLEMENTATION` is defined only once (in stb_image_impl.c)

### File Not Found Errors
- Solution: Use absolute paths with backslashes
- Example: `"c:\Users\Jason\Documents\Visual Studio\FreeIntvProjects\FreeIntvWorkingKeypad\src\libretro.c"`

## Next Steps

1. ✅ Build the project (done - `.dll` compiled)
2. Test in RetroArch with Intellivision ROMs
3. Verify horizontal layout displays correctly
4. Create game-specific overlay images
5. Optimize hotspot positioning as needed
6. Add touch input mapping for mobile/handheld devices

---

**Last Updated**: October 20, 2025
**Platform**: Windows 64-bit only
**Status**: Ready for development and testing
