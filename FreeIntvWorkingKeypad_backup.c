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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
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

// Include stb_image for PNG/JPG loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224

// ========================================
// HORIZONTAL LAYOUT DISPLAY CONFIGURATION
// ========================================
// Game Screen: Left side (704×448, 2x scaled from 352×224)
// Keypad: Right side (370×600)
// Utility Buttons: Below game on left side (6 buttons × ~62px each)
// Total Workspace: ~1100 × 968 pixels

#define WORKSPACE_WIDTH 1100    // Game (704) + Keypad (370) + margin (26)
#define WORKSPACE_HEIGHT 968    // Max(448, 620) + utilities (310) + margin
#define GAME_SCREEN_WIDTH 704   // 352 * 2x
#define GAME_SCREEN_HEIGHT 448  // 224 * 2x
#define KEYPAD_WIDTH 370        // Keypad overlay width
#define KEYPAD_HEIGHT 600       // Keypad overlay height
#define UTILITY_BUTTON_WIDTH 60
#define UTILITY_BUTTON_HEIGHT 50

// Keypad hotspot configuration
#define OVERLAY_HOTSPOT_COUNT 12
#define OVERLAY_HOTSPOT_SIZE 70

// RetroArch utility button codes
#define RETROARCH_MENU 1000
#define RETROARCH_PAUSE 1001
#define RETROARCH_REWIND 1002
#define RETROARCH_SAVE 1003
#define RETROARCH_LOAD 1004
#define RETROARCH_SWAP_OVERLAY 1005

#define UTILITY_BUTTON_COUNT 6

// Keypad button codes
#define K_1 0x02
#define K_2 0x03
#define K_3 0x04
#define K_4 0x13
#define K_5 0x23
#define K_6 0x14
#define K_7 0x84
#define K_8 0x44
#define K_9 0x24
#define K_0 0x48
#define K_C 0x88
#define K_E 0x28

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* label;
    int command;  // RetroArch command code
} utility_button_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int id;
    int keypad_code;
} overlay_hotspot_t;

overlay_hotspot_t overlay_hotspots[OVERLAY_HOTSPOT_COUNT];

// Utility buttons positioned below game screen on left side
static utility_button_t utility_buttons[UTILITY_BUTTON_COUNT] = {
    {10, 468, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "MENU", RETROARCH_MENU},
    {10, 528, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "PAUSE", RETROARCH_PAUSE},
    {10, 588, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "REWIND", RETROARCH_REWIND},
    {10, 648, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "SAVE", RETROARCH_SAVE},
    {10, 708, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "LOAD", RETROARCH_LOAD},
    {10, 768, UTILITY_BUTTON_WIDTH, UTILITY_BUTTON_HEIGHT, "<>", RETROARCH_SWAP_OVERLAY}
};

// Display system variables
static int dual_screen_enabled = 1;
static void* dual_screen_buffer = NULL;
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;

// PNG overlay system
static char current_rom_path[512] = {0};
static char system_dir[512] = {0};
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;
static int overlay_width = 370;
static int overlay_height = 600;

// Controller base
static unsigned int* controller_base = NULL;
static int controller_base_loaded = 0;
static int controller_base_width = 446;
static int controller_base_height = 620;

// Initialize overlay hotspots for keypad (positioned on RIGHT side)
static void init_overlay_hotspots(void)
{
    printf("[INIT] Initializing overlay hotspots (horizontal layout)...\n");
    fflush(stdout);
    
    // Layout: 4 rows x 3 columns, positioned on RIGHT side of workspace
    int hotspot_w = OVERLAY_HOTSPOT_SIZE;
    int hotspot_h = OVERLAY_HOTSPOT_SIZE;
    int gap_x = 28;
    int gap_y = 29;
    int rows = 4;
    int cols = 3;
    
    // Position keypad on right side: start at GAME_SCREEN_WIDTH
    int keypad_x_offset = GAME_SCREEN_WIDTH;
    int keypad_y_offset = 0;  // Align to top of keypad region
    
    // Center hotspots within keypad (370px wide)
    int right_margin = (KEYPAD_WIDTH - (3 * hotspot_w + 2 * gap_x)) / 2;
    int top_margin = 30;  // Some space from top of keypad
    
    int start_x = keypad_x_offset + right_margin;
    int start_y = keypad_y_offset + top_margin;
    
    int keypad_map[12] = { K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_C, K_0, K_E };
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = row * cols + col;
            overlay_hotspots[idx].x = start_x + col * (hotspot_w + gap_x);
            overlay_hotspots[idx].y = start_y + row * (hotspot_h + gap_y);
            overlay_hotspots[idx].width = hotspot_w;
            overlay_hotspots[idx].height = hotspot_h;
            overlay_hotspots[idx].id = idx + 1;
            overlay_hotspots[idx].keypad_code = keypad_map[idx];
            printf("[INIT] Hotspot %d: pos=(%d,%d), size=%dx%d, keypad_code=0x%02X\n",
                   idx, overlay_hotspots[idx].x, overlay_hotspots[idx].y,
                   overlay_hotspots[idx].width, overlay_hotspots[idx].height,
                   overlay_hotspots[idx].keypad_code);
        }
    }
    printf("[INIT] Hotspot initialization complete!\n");
    fflush(stdout);
}

// Load controller base PNG
static void load_controller_base(void)
{
    if (controller_base_loaded || !system_dir[0]) {
        return;
    }
    
    char base_path[512];
    snprintf(base_path, sizeof(base_path), "%s\\freeintvds-overlays\\controller_base.png", system_dir);
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(base_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        snprintf(base_path, sizeof(base_path), "%s\\freeintvds-overlays\\default.png", system_dir);
        img_data = stbi_load(base_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[CONTROLLER] Loaded controller base: %dx%d\n", width, height);
        controller_base_width = width;
        controller_base_height = height;
        
        if (!controller_base) {
            controller_base = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        }
        
        if (controller_base) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    controller_base[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            controller_base_loaded = 1;
            stbi_image_free(img_data);
            printf("[CONTROLLER] Controller base loaded successfully\n");
        }
    }
}

// Build overlay path from ROM name
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0 || system_dir[0] == '\0') {
        overlay_path[0] = '\0';
        return;
    }
    
    const char* filename = rom_path;
    const char* p = rom_path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    const char* ext = filename;
    const char* q = filename;
    while (*q) {
        if (*q == '.') {
            ext = q;
        }
        q++;
    }
    size_t name_len = ext - filename;
    
    snprintf(overlay_path, overlay_path_size, 
             "%s\\freeintvds-overlays\\%.*s.png",
             system_dir, (int)name_len, filename);
}

// Load overlay for ROM
static void load_overlay_for_rom(const char* rom_path)
{
    if (!rom_path || !dual_screen_enabled) return;
    
    char overlay_path[1024];
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    
    overlay_loaded = 0;
    
    if (overlay_buffer) {
        free(overlay_buffer);
        overlay_buffer = NULL;
    }
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        char jpg_path[1024];
        strncpy(jpg_path, overlay_path, sizeof(jpg_path) - 1);
        char* ext = strrchr(jpg_path, '.');
        if (ext) {
            strcpy(ext, ".jpg");
            img_data = stbi_load(jpg_path, &width, &height, &channels, 4);
        }
    }
    
    if (!img_data && system_dir[0] != '\0') {
        char default_path[1024];
        snprintf(default_path, sizeof(default_path), "%s\\freeintvds-overlays\\default.png", system_dir);
        img_data = stbi_load(default_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[OVERLAY] Loaded overlay: %dx%d\n", width, height);
        overlay_width = width;
        overlay_height = height;
        overlay_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    overlay_buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            init_overlay_hotspots();
        }
        stbi_image_free(img_data);
    } else {
        overlay_width = 370;
        overlay_height = 600;
        overlay_buffer = (unsigned int*)malloc(overlay_width * overlay_height * sizeof(unsigned int));
        if (overlay_buffer) {
            for (int y = 0; y < overlay_height; y++) {
                for (int x = 0; x < overlay_width; x++) {
                    if (y < overlay_height / 2 && x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF0000FF;
                    else if (y < overlay_height / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF00FF00;
                    else if (x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFFFF0000;
                    else
                        overlay_buffer[y * overlay_width + x] = 0xFFFFFFFF;
                }
            }
        }
    }
    
    overlay_loaded = 1;
    strncpy(current_rom_path, rom_path, sizeof(current_rom_path) - 1);
}

// Render display with game screen LEFT and keypad RIGHT
static void render_dual_screen(void)
{
    if (!dual_screen_enabled) return;
    
    if (!dual_screen_buffer) {
        dual_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
    }
    if (!dual_screen_buffer) return;
    
    unsigned int* dual_buffer = (unsigned int*)dual_screen_buffer;
    extern unsigned int frame[352 * 224];
    
    // Clear entire workspace with black
    for (int i = 0; i < WORKSPACE_WIDTH * WORKSPACE_HEIGHT; i++) {
        dual_buffer[i] = 0xFF000000;
    }
    
    // === GAME SCREEN (LEFT side, top-aligned) ===
    for (int y = 0; y < GAME_SCREEN_HEIGHT; ++y) {
        int src_y = y / 2;
        for (int x = 0; x < GAME_SCREEN_WIDTH; ++x) {
            int src_x = x / 2;
            if (src_y < GAME_HEIGHT && src_x < GAME_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = frame[src_y * GAME_WIDTH + src_x];
            } else {
                dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000;
            }
        }
    }
    
    // === KEYPAD (RIGHT side, top-aligned) ===
    // Background for keypad area
    unsigned int bg_color = 0xFF1a1a1a;
    for (int y = 0; y < KEYPAD_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = GAME_SCREEN_WIDTH + x;
            if (workspace_x < WORKSPACE_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = bg_color;
            }
        }
    }
    
    // Layer overlay and controller base
    int ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;
    int overlay_x_offset = (KEYPAD_WIDTH - overlay_width) / 2;
    
    for (int y = 0; y < KEYPAD_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = GAME_SCREEN_WIDTH + x;
            int workspace_y = y;
            
            if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
            
            unsigned int pixel = bg_color;
            
            // Layer game overlay (back)
            if (overlay_buffer && y < overlay_height) {
                int overlay_x = x - overlay_x_offset;
                if (overlay_x >= 0 && overlay_x < overlay_width) {
                    unsigned int overlay_pixel = overlay_buffer[y * overlay_width + overlay_x];
                    if ((overlay_pixel >> 24) & 0xFF) {
                        pixel = overlay_pixel;
                    }
                }
            }
            
            // Layer controller base (front)
            if (controller_base_loaded && controller_base && y < controller_base_height) {
                int ctrl_x = x - ctrl_base_x_offset;
                if (ctrl_x >= 0 && ctrl_x < controller_base_width) {
                    unsigned int base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    if ((base_pixel >> 24) & 0xFF) {
                        pixel = base_pixel;
                    }
                }
            }
            
            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = pixel;
        }
    }
    
    // === UTILITY BUTTONS (BELOW game screen, LEFT side) ===
    // Draw utility button guide rectangles
    unsigned int utility_color = 0xFFFFD700;
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        utility_button_t* btn = &utility_buttons[i];
        for (int y = btn->y; y < btn->y + btn->height; ++y) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = btn->x; x < btn->x + btn->width; ++x) {
                if (x >= WORKSPACE_WIDTH) break;
                dual_buffer[y * WORKSPACE_WIDTH + x] = utility_color;
            }
        }
    }
    
    // === HOTSPOT HIGHLIGHTING ===
    int current_key = Memory[0x1FE] ^ 0xFF;
    int active_hotspot = -1;
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
        if ((current_key & overlay_hotspots[i].keypad_code) == overlay_hotspots[i].keypad_code) {
            active_hotspot = i;
            break;
        }
    }
    
    if (active_hotspot >= 0 && active_hotspot < OVERLAY_HOTSPOT_COUNT) {
        overlay_hotspot_t *h = &overlay_hotspots[active_hotspot];
        unsigned int highlight_color = 0xAA00FF00;
        
        for (int y = h->y; y < h->y + h->height; ++y) {
            if (y >= WORKSPACE_HEIGHT) continue;
            for (int x = h->x; x < h->x + h->width; ++x) {
                if (x >= WORKSPACE_WIDTH) continue;
                
                unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                unsigned int alpha = (highlight_color >> 24) & 0xFF;
                unsigned int inv_alpha = 255 - alpha;
                
                unsigned int r = ((highlight_color >> 16) & 0xFF);
                unsigned int g = ((highlight_color >> 8) & 0xFF);
                unsigned int b = (highlight_color & 0xFF);
                
                unsigned int existing_r = ((existing >> 16) & 0xFF);
                unsigned int existing_g = ((existing >> 8) & 0xFF);
                unsigned int existing_b = (existing & 0xFF);
                
                unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                
                dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
            }
        }
    }
}

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
	check_variables(true);
	LoadGame(info->path);
	
	// Capture system directory and load overlays
	if (SystemPath && SystemPath[0]) {
		strncpy(system_dir, SystemPath, sizeof(system_dir) - 1);
		system_dir[sizeof(system_dir) - 1] = '\0';
		printf("[GAME] System directory: %s\n", system_dir);
		
		// Load controller base and ROM-specific overlay
		load_controller_base();
		load_overlay_for_rom(info->path);
		init_overlay_hotspots();
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
	
	// Render dual-screen display (game + keypad)
	render_dual_screen();
	
	// Send frame to libretro
	if (dual_screen_enabled && dual_screen_buffer) {
		Video(dual_screen_buffer, WORKSPACE_WIDTH, WORKSPACE_HEIGHT, sizeof(unsigned int) * WORKSPACE_WIDTH);
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
	info->library_name = "FreeIntv";
	info->library_version = "1.2";
	info->valid_extensions = "int|bin|rom";
	info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;

	memset(info, 0, sizeof(*info));
	
	// Report dimensions based on dual-screen mode
	if (dual_screen_enabled) {
		info->geometry.base_width   = WORKSPACE_WIDTH;
		info->geometry.base_height  = WORKSPACE_HEIGHT;
		info->geometry.max_width    = WORKSPACE_WIDTH;
		info->geometry.max_height   = WORKSPACE_HEIGHT;
		info->geometry.aspect_ratio = ((float)WORKSPACE_WIDTH) / ((float)WORKSPACE_HEIGHT);
	} else {
		info->geometry.base_width   = MaxWidth;
		info->geometry.base_height  = MaxHeight;
		info->geometry.max_width    = MaxWidth;
		info->geometry.max_height   = MaxHeight;
		info->geometry.aspect_ratio = ((float)MaxWidth) / ((float)MaxHeight);
	}

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = AUDIO_FREQUENCY;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void)
{
	libretro_supports_option_categories = false;
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
