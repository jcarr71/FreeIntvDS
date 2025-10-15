/*
	This file is part of FreeIntv.

	FreeIntv is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeIntv is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with FreeIntv; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// TEST: Minimal modification to see if any change breaks the core
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "libretro.h"
#include "libretro_core_options.h"
#include <file/file_path.h>
#include <retro_miscellaneous.h>

#include "intv.h"
#include "cp1610.h"
#include "memory.h"
#include "stic.h"
#include "psg.h"
#include "ivoice.h"
#include "controller.h"
#include "osd.h"

// DUAL-SCREEN IMPLEMENTATION - Using validated safe patterns
static int dual_screen_enabled = 1;
static void* test_buffer = NULL; // Keep for now, will become dual-screen buffer
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;
static const int OVERLAY_WIDTH = 256;
static const int DUAL_WIDTH = 608;   // 352 + 256
static const int DUAL_HEIGHT = 224;

// PNG overlay system
static char current_rom_path[512] = {0};
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;

// Extract ROM name and build overlay path - handle ZIP extraction
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0) return;
    
    // Check if this looks like a temp/extracted path
    bool is_temp_path = (strstr(rom_path, "temp") != NULL) || 
                       (strstr(rom_path, "Temp") != NULL) ||
                       (strstr(rom_path, "extract") != NULL);
    
    const char* filename = rom_path;
    const char* p = rom_path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    // Remove extension to get base name
    const char* ext = filename;
    const char* q = filename;
    while (*q) {
        if (*q == '.') {
            ext = q;
        }
        q++;
    }
    size_t name_len = ext - filename;
    
    if (is_temp_path) {
        // If extracted, try common ROM locations instead of temp path
        const char* common_paths[] = {
            "C:\\RetroArch-Win64\\roms\\intellivision\\overlays\\%.*s.jpg",
            "C:\\RetroArch-Win64\\roms\\overlays\\%.*s.jpg", 
            "C:\\RetroArch\\roms\\intellivision\\overlays\\%.*s.jpg",
            "C:\\RetroArch\\roms\\overlays\\%.*s.jpg",
            "roms\\intellivision\\overlays\\%.*s.jpg",
            "roms\\overlays\\%.*s.jpg"
        };
        
        // Try first common path as primary
        snprintf(overlay_path, overlay_path_size, common_paths[0], (int)name_len, filename);
    } else {
        // Normal path - build relative to ROM directory
        size_t dir_len = filename - rom_path;
        snprintf(overlay_path, overlay_path_size, "%.*sOverlays\\%.*s.jpg", 
                 (int)dir_len, rom_path, (int)name_len, filename);
    }
}

// Load overlay for current ROM (placeholder implementation)
static void load_overlay_for_rom(const char* rom_path)
{
    // VERY FIRST THING - write to debug log to prove new code is running
    FILE* early_debug = fopen("freeintvds_debug.txt", "a");
    if (early_debug) {
        fprintf(early_debug, "=== NEW BUILD v3 LOADED ===\n");
        fclose(early_debug);
    }
    
    if (!rom_path || !dual_screen_enabled) return;
    
    // Build overlay file path in ROM directory's overlays subfolder
    char overlay_path[1024];
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    
    // Debug: Log the paths being used (you can remove this later)
    printf("[DEBUG] ROM path: %s\n", rom_path);
    printf("[DEBUG] Overlay path: %s\n", overlay_path);
    
    // Reset overlay state
    overlay_loaded = 0;
    
    // Allocate overlay buffer if needed
    if (!overlay_buffer) {
        overlay_buffer = (unsigned int*)malloc(OVERLAY_WIDTH * GAME_HEIGHT * sizeof(unsigned int));
    }
    
    if (overlay_buffer) {
        // Try to load image using stb_image, but with simpler processing
        int width, height, channels;
        unsigned char* img_data = NULL;
        
        // Create a simple debug log file we can easily find
        FILE* debug_log = fopen("freeintvds_debug.txt", "a");
        if (debug_log) {
            fprintf(debug_log, "\n=== FreeIntvDS Debug ===\n");
            fprintf(debug_log, "ROM path: '%s'\n", rom_path);
            fprintf(debug_log, "Primary overlay path: '%s'\n", overlay_path);
            fclose(debug_log);
        }
        
        // Try multiple paths with detailed logging
        printf("[DEBUG] ROM path: '%s'\n", rom_path);
        printf("[DEBUG] Primary overlay path: '%s'\n", overlay_path);
        
        img_data = stbi_load(overlay_path, &width, &height, &channels, 0); // Load in original format
        
        if (!img_data) {
            // Try multiple common ROM locations since RetroArch extracted to temp
            char fallback_path[1024];
            
            debug_log = fopen("freeintvds_debug.txt", "a");
            
            // Get just the filename without extension
            const char* filename = strrchr(rom_path, '\\');
            if (!filename) filename = strrchr(rom_path, '/');
            if (filename) filename++; else filename = rom_path;
            
            const char* ext = strrchr(filename, '.');
            size_t name_len = ext ? (ext - filename) : strlen(filename);
            
            // Try common RetroArch ROM directory structures
            const char* overlay_patterns[] = {
                "C:\\RetroArch-Win64\\roms\\intellivision\\overlays\\%.*s.jpg",
                "C:\\RetroArch-Win64\\roms\\intellivision\\overlays\\%.*s.png",
                "C:\\RetroArch-Win64\\roms\\overlays\\%.*s.jpg",
                "C:\\RetroArch-Win64\\roms\\overlays\\%.*s.png",
                "C:\\RetroArch\\roms\\intellivision\\overlays\\%.*s.jpg", 
                "C:\\RetroArch\\roms\\intellivision\\overlays\\%.*s.png",
                "C:\\RetroArch\\roms\\overlays\\%.*s.jpg",
                "C:\\RetroArch\\roms\\overlays\\%.*s.png",
                "roms\\intellivision\\overlays\\%.*s.jpg",
                "roms\\intellivision\\overlays\\%.*s.png",
                "roms\\overlays\\%.*s.jpg", 
                "roms\\overlays\\%.*s.png",
                NULL
            };
            
            for (int i = 0; overlay_patterns[i] != NULL; i++) {
                snprintf(fallback_path, sizeof(fallback_path), overlay_patterns[i], (int)name_len, filename);
                printf("[DEBUG] Trying pattern %d: '%s'\n", i+1, fallback_path);
                if (debug_log) fprintf(debug_log, "Trying pattern %d: '%s'\n", i+1, fallback_path);
                
                img_data = stbi_load(fallback_path, &width, &height, &channels, 0); // Load in original format
                if (img_data) {
                    printf("[DEBUG] SUCCESS: Found image at '%s'\n", fallback_path);
                    if (debug_log) fprintf(debug_log, "SUCCESS: Found image at '%s'\n", fallback_path);
                    break;
                }
            }
            
            if (!img_data && debug_log) {
                fprintf(debug_log, "No overlay found at any common location\n");
            }
            
            if (debug_log) fclose(debug_log);
        } else {
            printf("[DEBUG] SUCCESS: Found image at primary path\n");
            debug_log = fopen("freeintvds_debug.txt", "a");
            if (debug_log) {
                fprintf(debug_log, "SUCCESS: Found image at primary path\n");
                fclose(debug_log);
            }
        }
        
        if (img_data) {
            printf("[DEBUG] Image loaded successfully: %dx%d, %d channels\n", width, height, channels);
            
            // Add detailed debugging about the image data
            debug_log = fopen("freeintvds_debug.txt", "a");
            if (debug_log) {
                fprintf(debug_log, "Image dimensions: %dx%d, channels=%d (BUILD-v2)\n", width, height, channels);
                fprintf(debug_log, "First few pixels: ");
                if (channels == 3) {
                    for (int i = 0; i < 9 && i < width * height * channels; i += channels) {
                        fprintf(debug_log, "(%d,%d,%d) ", img_data[i], img_data[i+1], img_data[i+2]);
                    }
                } else if (channels == 4) {
                    for (int i = 0; i < 12 && i < width * height * channels; i += channels) {
                        fprintf(debug_log, "(%d,%d,%d,%d) ", img_data[i], img_data[i+1], img_data[i+2], img_data[i+3]);
                    }
                } else {
                    for (int i = 0; i < 3 && i < width * height * channels; i += channels) {
                        fprintf(debug_log, "(%d) ", img_data[i]);
                    }
                }
                fprintf(debug_log, "\n");
                fclose(debug_log);
            }
            
            // Add pixel processing debug to file log
            debug_log = fopen("freeintvds_debug.txt", "a");
            if (debug_log) {
                fprintf(debug_log, "Starting pixel processing loop...\n");
                fclose(debug_log);
            }
            
            printf("[DEBUG] PNG loaded: %dx%d, %d channels (BUILD-v2)\n", width, height, channels);
            printf("[DEBUG] Expected display area: %dx%d\n", OVERLAY_WIDTH, GAME_HEIGHT);
            printf("[DEBUG] Starting pixel processing loop...\n");
            
            // Add detailed debugging about coordinate mapping
            debug_log = fopen("freeintvds_debug.txt", "a");
            if (debug_log) {
                fprintf(debug_log, "PNG dimensions: %dx%d\n", width, height);
                fprintf(debug_log, "Display area: %dx%d\n", OVERLAY_WIDTH, GAME_HEIGHT);
                fprintf(debug_log, "Expected pixels: %d\n", width * height);
                fclose(debug_log);
            }
            
            // Now display the actual PNG with VERY careful bounds checking
            int pixels_processed = 0;
            int pixels_in_bounds = 0;
            
            for (int y = 0; y < GAME_HEIGHT; y++) {
                int row_pixels_processed = 0;
                for (int x = 0; x < OVERLAY_WIDTH; x++) {
                    // Display PNG with 1:1 mapping (no scaling)
                    int png_x = x;
                    int png_y = y;  // Direct 1:1 mapping, no offset
                    
                    // Very conservative bounds checking
                    if (png_x >= 0 && png_x < width && png_y >= 0 && png_y < height) {
                        pixels_in_bounds++;
                        row_pixels_processed++;
                        
                        // Calculate source index based on actual channel count
                        int src_index = (png_y * width + png_x) * channels;
                        int max_index = width * height * channels;
                        
                        if (src_index >= 0 && src_index + (channels-1) < max_index) {
                            pixels_processed++;
                                
                                // Extract pixel data based on channel count
                                unsigned char r, g, b;
                                
                                if (channels == 1) {
                                    // Grayscale
                                    r = g = b = img_data[src_index];
                                } else if (channels == 3) {
                                    // RGB
                                    r = img_data[src_index + 0];
                                    g = img_data[src_index + 1];
                                    b = img_data[src_index + 2];
                                } else if (channels == 4) {
                                    // RGBA (ignore alpha for now)
                                    r = img_data[src_index + 0];
                                    g = img_data[src_index + 1];
                                    b = img_data[src_index + 2];
                                    // unsigned char a = img_data[src_index + 3];
                                } else {
                                    // Unknown format - use red to indicate error
                                    r = 255; g = 0; b = 0;
                                }
                                
                                // Debug first few pixels to verify data (minimal logging)
                                if (png_x == 0 && png_y == 0) {
                                    printf("[PIXEL] First pixel RGB(%d,%d,%d)\n", r, g, b);
                                }
                                
                                // Simple ARGB format (no bit shifting tricks)
                                unsigned int pixel = 0xFF000000;  // Alpha = 255
                                pixel |= ((unsigned int)r) << 16;  // Red
                                pixel |= ((unsigned int)g) << 8;   // Green  
                                pixel |= ((unsigned int)b);        // Blue
                                
                                overlay_buffer[y * OVERLAY_WIDTH + x] = pixel;
                            } else {
                                // Bounds error - bright red
                                overlay_buffer[y * OVERLAY_WIDTH + x] = 0xFFFF0000;
                            }
                        } else {
                            // Outside PNG bounds - dark blue  
                            overlay_buffer[y * OVERLAY_WIDTH + x] = 0xFF000080;
                        }
                    }
                }
            }
            
            printf("[DEBUG] PNG overlay processing completed successfully\n");
            
            // Add results to file log
            debug_log = fopen("freeintvds_debug.txt", "a");
            if (debug_log) {
                fprintf(debug_log, "PNG overlay processing completed\n");
                fclose(debug_log);
            }
            
            stbi_image_free(img_data);
        } else {
            printf("[DEBUG] No image found, using test pattern\n");
            
            // If no image, create a simple Intellivision keypad layout
            for (int y = 0; y < GAME_HEIGHT; y++) {
                for (int x = 0; x < OVERLAY_WIDTH; x++) {
                    // Create a simple 4x3 grid for Intellivision keypad
                    int grid_x = x / (OVERLAY_WIDTH / 4);  // 0-3 
                    int grid_y = y / (GAME_HEIGHT / 3);   // 0-2
                    
                    // Keypad numbers: 1,2,3,C / 4,5,6,0 / 7,8,9,E
                    unsigned int colors[] = {
                        0xFF400000, 0xFF004000, 0xFF000040, 0xFF404000,  // Row 1: dark colors
                        0xFF400040, 0xFF004040, 0xFF202020, 0xFF402020,  // Row 2: medium colors  
                        0xFF204020, 0xFF202040, 0xFF404020, 0xFF204040   // Row 3: mixed colors
                    };
                    
                    int color_index = grid_y * 4 + grid_x;
                    if (color_index < 12) {
                        overlay_buffer[y * OVERLAY_WIDTH + x] = colors[color_index];
                    } else {
                        overlay_buffer[y * OVERLAY_WIDTH + x] = 0xFF101010; // Very dark gray
                    }
                }
            }
        }
        
        overlay_loaded = 1;
        strncpy(current_rom_path, rom_path, sizeof(current_rom_path) - 1);
    }

// Safe dual-screen function using proven patterns
static void test_function(void)
{
    if (!dual_screen_enabled) return;
    
    // Allocate dual-screen buffer using proven safe malloc pattern
    if (!test_buffer) {
        test_buffer = malloc(DUAL_WIDTH * DUAL_HEIGHT * sizeof(unsigned int));
    }
    
    if (test_buffer) {
        unsigned int* dual_buffer = (unsigned int*)test_buffer;
        extern unsigned int frame[352 * 224];
        
        // Copy game screen to left side of dual buffer (proven safe pattern)
        for (int y = 0; y < GAME_HEIGHT; y++) {
            for (int x = 0; x < GAME_WIDTH; x++) {
                dual_buffer[y * DUAL_WIDTH + x] = frame[y * GAME_WIDTH + x];
            }
        }
        
        // Draw overlay on right side - use loaded overlay if available
        for (int y = 0; y < DUAL_HEIGHT; y++) {
            for (int x = 0; x < OVERLAY_WIDTH; x++) {
                int dual_x = GAME_WIDTH + x;  // Right side starts at x=352
                
                unsigned int overlay_pixel;
                if (overlay_loaded && overlay_buffer) {
                    // Use loaded Intellivision keypad overlay
                    overlay_pixel = overlay_buffer[y * OVERLAY_WIDTH + x];
                } else {
                    // Fallback checkerboard pattern for visibility
                    if ((x / 32 + y / 32) % 2 == 0) {
                        overlay_pixel = 0xFF8000FF; // Purple
                    } else {
                        overlay_pixel = 0xFF404040; // Dark gray
                    }
                }
                
                dual_buffer[y * DUAL_WIDTH + dual_x] = overlay_pixel;
            }
        }
    }
    
    return;
}

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224

char *SystemPath;

retro_environment_t Environ;
retro_video_refresh_t Video;
retro_audio_sample_t Audio;
retro_audio_sample_batch_t AudioBatch;
retro_input_poll_t InputPoll;
retro_input_state_t InputState;

void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
void retro_set_audio_sample(retro_audio_sample_t fn) { Audio = fn; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t fn) { AudioBatch = fn; }
void retro_set_input_poll(retro_input_poll_t fn) { InputPoll = fn; }
void retro_set_input_state(retro_input_state_t fn) { InputState = fn; }

struct retro_game_geometry Geometry;

static bool libretro_supports_option_categories = false;

int joypad0[20]; // joypad 0 state
int joypad1[20]; // joypad 1 state
int joypre0[20]; // joypad 0 previous state
int joypre1[20]; // joypad 1 previous state

bool paused = false;

bool keyboardChange = false;
bool keyboardDown = false;
int  keyboardState = 0;

// at 44.1khz, read 735 samples (44100/60) 
// at 48khz, read 800 samples (48000/60)
// e.g. audioInc = 3733.5 / 735
int audioSamples = AUDIO_FREQUENCY / 60;

double audioBufferPos = 0.0;
double audioInc;

double ivoiceBufferPos = 0.0;
double ivoiceInc;

unsigned int frameWidth = MaxWidth;
unsigned int frameHeight = MaxHeight;
unsigned int frameSize =  MaxWidth * MaxHeight; //78848

void quit(int state)
{
	Reset();
	MemoryInit();
}

static void Keyboard(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers)
{
	/* Keyboard Input */
	keyboardDown = down;
	keyboardChange = true; 
	switch (character)
	{
		case 48: keyboardState = keypadStates[10]; break; // 0
		case 49: keyboardState = keypadStates[0]; break; // 1
		case 50: keyboardState = keypadStates[1]; break; // 2
		case 51: keyboardState = keypadStates[2]; break; // 3
		case 52: keyboardState = keypadStates[3]; break; // 4
		case 53: keyboardState = keypadStates[4]; break; // 5
		case 54: keyboardState = keypadStates[5]; break; // 6
		case 55: keyboardState = keypadStates[6]; break; // 7
		case 56: keyboardState = keypadStates[7]; break; // 8
		case 57: keyboardState = keypadStates[8]; break; // 9
		case 91: keyboardState = keypadStates[9]; break; // C [
		case 93: keyboardState = keypadStates[11]; break; // E ]
		default: 
			keyboardChange = false;
			keyboardDown = false;
	}
}

static void check_variables(bool first_run)
{
	struct retro_variable var = {0};

	if (first_run)
	{
		var.key   = "default_p1_controller";
		var.value = NULL;

		// by default input 0 maps to Right Controller (0x1FE)
		// and input 1 maps to Left Controller (0x1FF)
		controllerSwap = 0;

		if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "left") == 0)
				controllerSwap = 1;
		}
	}
}

void retro_set_environment(retro_environment_t fn)
{
	Environ = fn;

	// Set core options
	libretro_supports_option_categories = false;
	libretro_set_core_options(Environ, &libretro_supports_option_categories);
}

void retro_init(void)
{
	char execPath[PATH_MAX_LENGTH];
	char gromPath[PATH_MAX_LENGTH];
	struct retro_keyboard_callback kb = { Keyboard };

	// controller descriptors
	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 0 },
	};

	// init buffers, structs
	memset(frame, 0, frameSize);
	OSD_setDisplay(frame, MaxWidth, MaxHeight);

	Environ(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	// reset console
	Init();
	Reset();

	// get paths
	Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath);

	// load exec
	fill_pathname_join(execPath, SystemPath, "exec.bin", PATH_MAX_LENGTH);
	loadExec(execPath);

	// load grom
	fill_pathname_join(gromPath, SystemPath, "grom.bin", PATH_MAX_LENGTH);
	loadGrom(gromPath);

	// Setup keyboard input
	Environ(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb);
}

bool retro_load_game(const struct retro_game_info *info)
{
	// Simpler debug approach - try multiple paths
	FILE* debug_log = fopen("C:\\temp\\freeintvds_debug.txt", "w");
	if (!debug_log) debug_log = fopen("freeintvds_debug.txt", "w");
	if (!debug_log) debug_log = fopen("debug.txt", "w");
	
	if (debug_log) {
		fprintf(debug_log, "=== FreeIntvDS Load Game Debug ===\n");
		fprintf(debug_log, "Function called successfully\n");
		if (info && info->path) {
			fprintf(debug_log, "ROM path: '%s'\n", info->path);
			fprintf(debug_log, "Dual screen: %s\n", dual_screen_enabled ? "YES" : "NO");
		} else {
			fprintf(debug_log, "No ROM info provided\n");
		}
		fflush(debug_log);
		fclose(debug_log);
	}
	
	check_variables(true);
	LoadGame(info->path);
	
	// Load overlay for the current ROM
	if (dual_screen_enabled && info && info->path) {
		load_overlay_for_rom(info->path);
	}
	
	return true;
}

void retro_unload_game(void)
{
	quit(0);
}

void retro_run(void)
{
	int c, i, j, k, l;
	int showKeypad0 = false;
	int showKeypad1 = false;

	// TEST: Call our simple function to see if function calls break the core
	test_function();

	bool options_updated  = false;
	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
		check_variables(false);

	InputPoll();

	for(i = 0; i < 20; i++) // Copy previous state
	{
		joypre0[i] = joypad0[i];
		joypre1[i] = joypad1[i];
	}

	/* JoyPad 0 */
	joypad0[0] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad0[1] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad0[2] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad0[3] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad0[4] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad0[5] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad0[6] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad0[7] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad0[8] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad0[9] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad0[10] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad0[11] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad0[12] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad0[13] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad0[14] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[15] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[18] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad0[19] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	/* JoyPad 1 */
	joypad1[0] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad1[1] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad1[2] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad1[3] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad1[4] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad1[5] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad1[6] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad1[7] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad1[8] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad1[9] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad1[10] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad1[11] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad1[12] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad1[13] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad1[14] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[15] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[18] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad1[19] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	// Pause
	if((joypad0[8]==1 && joypre0[8]==0) || (joypad1[8]==1 && joypre1[8]==0))
	{
		paused = !paused;
		if(paused)
		{
			OSD_drawPaused();
			OSD_drawTextCenterBG(21, "HELP - PRESS A");
		}
	}

	if(paused)
	{
		// help menu //
		if(joypad0[4]==1 || joypad1[4]==1)
		{
			OSD_drawTextBG(3,  4, "                                      ");
			OSD_drawTextBG(3,  5, "               - HELP -               ");
			OSD_drawTextBG(3,  6, "                                      ");
			OSD_drawTextBG(3,  7, " A      - RIGHT ACTION BUTTON         ");
			OSD_drawTextBG(3,  8, " B      - LEFT ACTION BUTTON          ");
			OSD_drawTextBG(3,  9, " Y      - TOP ACTION BUTTON           ");
			OSD_drawTextBG(3, 10, " X      - LAST SELECTED KEYPAD BUTTON ");
			OSD_drawTextBG(3, 11, " L/R    - SHOW KEYPAD                 ");
			OSD_drawTextBG(3, 12, " LT/RT  - KEYPAD CLEAR/ENTER          ");
			OSD_drawTextBG(3, 13, "                                      ");
			OSD_drawTextBG(3, 14, " START  - PAUSE GAME                  ");
			OSD_drawTextBG(3, 15, " SELECT - SWAP LEFT/RIGHT CONTROLLERS ");
			OSD_drawTextBG(3, 16, "                                      ");
			OSD_drawTextBG(3, 17, " FREEINTV 1.2          LICENSE GPL V2+");
			OSD_drawTextBG(3, 18, "                                      ");
		}
	}
	else
	{
		if(joypad0[10] | joypad0[11]) // left/right shoulder down
		{
			showKeypad0 = true;
			setControllerInput(0, getKeypadState(0, joypad0, joypre0));
		}
		else
		{
			showKeypad0 = false;
			setControllerInput(0, getControllerState(joypad0, 0));
		}

		if(joypad1[10] | joypad1[11]) // left shoulder down
		{
			showKeypad1 = true;
			setControllerInput(1, getKeypadState(1, joypad1, joypre1));
		}
		else
		{
			showKeypad1 = false;
			setControllerInput(1, getControllerState(joypad1, 1));
		}

		if(keyboardDown || keyboardChange)
		{
			setControllerInput(0, keyboardState);
			keyboardChange = false;
		}

		// grab frame
		Run();

		// draw overlays
		if(showKeypad0) { drawMiniKeypad(0, frame); }
		if(showKeypad1) { drawMiniKeypad(1, frame); }

		// sample audio from buffer
		audioInc = 3733.5 / audioSamples;
		ivoiceInc = 1.0;

		j = 0;
		for(i=0; i<audioSamples; i++)
		{
			// Sound interpolator:
			//   The PSG module generates audio at 224010 hz (3733.5 samples per frame)
			//   Very high frequencies like 0x0001 would generate chirps on output
			//   (For example, Lock&Chase) so this code interpolates audio, making
			//   these silent as in real hardware.
			audioBufferPos += audioInc;
			k = audioBufferPos;
			l = k - j;

			c = 0;
			while (j < k)
				c += PSGBuffer[j++];
			c = c / l;
			// Finally it adds the Intellivoice output (properly generated at the
			// same frequency as output)
			c = (c + ivoiceBuffer[(int) ivoiceBufferPos]) / 2;

			Audio(c, c); // Audio(left, right)

			ivoiceBufferPos += ivoiceInc;

			if (ivoiceBufferPos >= ivoiceBufferSize)
				ivoiceBufferPos = 0.0;

			audioBufferPos = audioBufferPos * (audioBufferPos<(PSGBufferSize-1));
		}
		audioBufferPos = 0.0;
		PSGFrame();
		ivoiceBufferPos = 0.0;
		ivoice_frame();
	}

	// Swap Left/Right Controller
	if(joypad0[9]==1 || joypad1[9]==1)
	{
		if ((joypad0[9]==1 && joypre0[9]==0) || (joypad1[9]==1 && joypre1[9]==0))
		{
			controllerSwap = controllerSwap ^ 1;
		}
		if(controllerSwap==1)
		{
			OSD_drawLeftRight();
		}
		else
		{
			OSD_drawRightLeft();
		}
	}

	if (intv_halt)
		OSD_drawTextBG(3, 5, "INTELLIVISION HALTED");
	
	// Send frame to libretro - use dual-screen buffer if enabled
	if (dual_screen_enabled && test_buffer) {
		Video(test_buffer, DUAL_WIDTH, DUAL_HEIGHT, sizeof(unsigned int) * DUAL_WIDTH);
	} else {
		Video(frame, frameWidth, frameHeight, sizeof(unsigned int) * frameWidth);
	}

}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
#ifdef FREEINTV_DS
	info->library_name = "FreeIntvDS";
	info->library_version = "1.2-ds";
#else
	info->library_name = "FreeIntv";
	info->library_version = "1.2";
#endif
	info->valid_extensions = "int|bin|rom";
	info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;

	// Use dual-screen dimensions using proven safe pattern
	int width = dual_screen_enabled ? DUAL_WIDTH : MaxWidth;
	int height = dual_screen_enabled ? DUAL_HEIGHT : MaxHeight;

	memset(info, 0, sizeof(*info));
	info->geometry.base_width   = width;
	info->geometry.base_height  = height;
	info->geometry.max_width    = width;
	info->geometry.max_height   = height;
	info->geometry.aspect_ratio = ((float)width) / ((float)height);

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = AUDIO_FREQUENCY;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void)
{
	libretro_supports_option_categories = false;
	
	// Clean up dual-screen buffers
	if (test_buffer) {
		free(test_buffer);
		test_buffer = NULL;
	}
	
	if (overlay_buffer) {
		free(overlay_buffer);
		overlay_buffer = NULL;
	}
	
	quit(0);
}

void retro_reset(void)
{
	// Reset (from intv.c) //
	Reset();
}

RETRO_API void *retro_get_memory_data(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return Memory;
	}
	return 0;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return 0x10000;
	}
	return 0;
}

#define SERIALIZED_VERSION 0x4f544702

struct serialized {
	int version;
	struct CP1610serialized CP1610;
	struct STICserialized STIC;
	struct PSGserialized PSG;
	struct ivoiceSerialized ivoice;
	unsigned int Memory[0x10000];   // Should be equal to Memory.c
	// Extra variables from intv.c
	int SR1;
	int intv_halt;
};

size_t retro_serialize_size(void)
{
	return sizeof(struct serialized);
}

bool retro_serialize(void *data, size_t size)
{
	struct serialized *all;

	all = (struct serialized *) data;
	all->version = SERIALIZED_VERSION;
	CP1610Serialize(&all->CP1610);
	STICSerialize(&all->STIC);
	PSGSerialize(&all->PSG);
	ivoiceSerialize(&all->ivoice);
	memcpy(all->Memory, Memory, sizeof(Memory));
	all->SR1 = SR1;
	all->intv_halt = intv_halt;
	return true;
}

bool retro_unserialize(const void *data, size_t size)
{
	const struct serialized *all;

	all = (const struct serialized *) data;
	if (all->version != SERIALIZED_VERSION)
		return false;
	CP1610Unserialize(&all->CP1610);
	STICUnserialize(&all->STIC);
	PSGUnserialize(&all->PSG);
	ivoiceUnserialize(&all->ivoice);
	memcpy(Memory, all->Memory, sizeof(Memory));
	SR1 = all->SR1;
	intv_halt = all->intv_halt;
	return true;
}

/* Stubs */
unsigned int retro_api_version(void) { return RETRO_API_VERSION; }
void retro_cheat_reset(void) {  }
void retro_cheat_set(unsigned index, bool enabled, const char *code) {  }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {  }
