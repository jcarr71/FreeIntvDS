bool libretro_supports_option_categories = false;
#include "deps/libretro-common/include/file/file_path.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "stb_image.h"
#include "cp1610.h"
#include "stic.h"
#include "psg.h"
#define AUDIO_FREQUENCY 44100
#include "ivoice.h"
#include "libretro_core_options.h"
#include "deps/libretro-common/include/libretro.h"
#include "intv.h"
#include "memory.h"
#include "osd.h"
#include "psg.h"
#include "ivoice.h"
#include "stic.h"
#ifndef AUDIO_FREQUENCY
#define AUDIO_FREQUENCY 44100
#endif

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096
#endif


extern bool libretro_supports_option_categories;

// Intellivision keypad button codes
#define K_1 0x81
#define K_2 0x41
#define K_3 0x21
#define K_4 0x82
#define K_5 0x42
#define K_6 0x22
#define K_7 0x84
#define K_8 0x44
#define K_9 0x24
#define K_0 0x48
#define K_C 0x88
#define K_E 0x28

#define OVERLAY_HOTSPOT_COUNT 12
#define OVERLAY_HOTSPOT_SIZE 70

typedef struct {
    int x; // Top-left X coordinate
    int y; // Top-left Y coordinate
    int width;
    int height;
    int id; // Hotspot ID (1-12)
    int keypad_code; // Intellivision keypad constant
} overlay_hotspot_t;

overlay_hotspot_t overlay_hotspots[OVERLAY_HOTSPOT_COUNT];

// Debug: draw rectangles for each hotspot on overlay buffer
static void debug_render_hotspots(unsigned int *buffer, int buf_width, int buf_height)
{
    unsigned int color = 0x80FF0000; // Semi-transparent red (ARGB)
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
        overlay_hotspot_t *h = &overlay_hotspots[i];
        // Draw top and bottom borders
        for (int x = h->x; x < h->x + h->width; x++) {
            if (h->y >= 0 && h->y < buf_height && x >= 0 && x < buf_width)
                buffer[h->y * buf_width + x] = color;
            if ((h->y + h->height - 1) >= 0 && (h->y + h->height - 1) < buf_height && x >= 0 && x < buf_width)
                buffer[(h->y + h->height - 1) * buf_width + x] = color;
        }
        // Draw left and right borders
        for (int y = h->y; y < h->y + h->height; y++) {
            if (y >= 0 && y < buf_height && h->x >= 0 && h->x < buf_width)
                buffer[y * buf_width + h->x] = color;
            if (y >= 0 && y < buf_height && (h->x + h->width - 1) >= 0 && (h->x + h->width - 1) < buf_width)
                buffer[y * buf_width + (h->x + h->width - 1)] = color;
        }
    }
}

// Initialize overlay hotspots (call after overlay dimensions are known)
static void init_overlay_hotspots(int overlay_width, int overlay_height)
{
    // Layout: 4 rows x 3 columns
    // Top row (row 0) starts 183 px from top
    // Rightmost column (col 2) is 89 px from right
    // Each hotspot is 70x70 px
    // Horizontal gap: 28 px, vertical gap: 29 px
    int hotspot_w = OVERLAY_HOTSPOT_SIZE;
    int hotspot_h = OVERLAY_HOTSPOT_SIZE;
    int gap_x = 28;
    int gap_y = 29;
    int rows = 4;
    int cols = 3;
    int start_y = 183;
    int right_margin = 89;
    int start_x = overlay_width - right_margin - (hotspot_w * cols) - (gap_x * (cols - 1));
    // Keypad mapping: 1-9 (first 3 rows), Clear-0-Enter (last row)
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
        }
    }
}

#include "controller.h"

// Workspace: game screen doubled (704x448) + overlay (704x620)
#define WORKSPACE_WIDTH 704
#define WORKSPACE_HEIGHT 1068  // 448 + 620
#define GAME_SCREEN_HEIGHT 448  // Doubled from 224
#define OVERLAY_HEIGHT 620  // Match controller base height

// DUAL-SCREEN IMPLEMENTATION
static int dual_screen_enabled = 1;
static void* dual_screen_buffer = NULL;
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;

// PNG overlay system
static char current_rom_path[512] = {0};
static char system_dir[512] = {0};  // RetroArch system directory
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;
static int overlay_width = 370;   // Actual overlay width
static int overlay_height = 600;  // Actual overlay height

// Controller base image system
static unsigned int* controller_base = NULL;  // Static controller image with transparent windows
static int controller_base_loaded = 0;
static int controller_base_width = 446;   // Controller base actual width
static int controller_base_height = 620;  // Controller base actual height (full overlay region)

// Load the static controller base image (called once at startup)
static void load_controller_base(void)
{
    if (controller_base_loaded || !system_dir[0]) {
        return;
    }
    
    // Try to load controller_base.png from system/freeintvds-overlays/
    char base_path[512];
    snprintf(base_path, sizeof(base_path), "%s\\freeintvds-overlays\\controller_base.png", system_dir);
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(base_path, &width, &height, &channels, 4);
    
    // If controller_base.png not found, try default.png
    if (!img_data) {
        snprintf(base_path, sizeof(base_path), "%s\\freeintvds-overlays\\default.png", system_dir);
        img_data = stbi_load(base_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[CONTROLLER] Loaded controller base image: %dx%d from %s\n", width, height, base_path);
        
        // Store actual dimensions
        controller_base_width = width;
        controller_base_height = height;
        
        // Allocate controller base buffer at native size
        if (!controller_base) {
            controller_base = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        }
        
        if (controller_base) {
            // Copy controller base at native resolution (no scaling)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    
                    // Store as ARGB with original alpha channel
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                        controller_base[y * width + x] = 
                            (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            
            controller_base_loaded = 1;
            stbi_image_free(img_data);
            printf("[CONTROLLER] Controller base stored at native %dx%d resolution\n", controller_base_width, controller_base_height);
        }
    } else {
        printf("[CONTROLLER] No controller base image found at %s, will use default\n", base_path);
    }
}

// Extract ROM name and build overlay path - handle ZIP extraction
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0 || system_dir[0] == '\0') {
        overlay_path[0] = '\0';
        return;
    }
    
    // Extract just the filename (without path or extension)
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
    
    // Build path in system/freeintvds-overlays/
    // Try PNG first
    snprintf(overlay_path, overlay_path_size, 
             "%s\\freeintvds-overlays\\%.*s.png",
             system_dir, (int)name_len, filename);
}

// Load overlay for current ROM
static void load_overlay_for_rom(const char* rom_path)
{
    if (!rom_path || !dual_screen_enabled) return;
    
    // Build overlay file path in system/freeintvds-overlays/
    char overlay_path[1024];
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    
    // Reset overlay state
    overlay_loaded = 0;
    
    // Free existing overlay buffer if dimensions might change
    if (overlay_buffer) {
        free(overlay_buffer);
        overlay_buffer = NULL;
    }
    
    int width, height, channels;
    unsigned char* img_data = NULL;
    
    // Try PNG first
    img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
    
    // If PNG not found, try JPG
    if (!img_data) {
        char jpg_path[1024];
        strncpy(jpg_path, overlay_path, sizeof(jpg_path) - 1);
        jpg_path[sizeof(jpg_path) - 1] = '\0';
        
        char* ext = strrchr(jpg_path, '.');
        if (ext && strcmp(ext, ".png") == 0) {
            strcpy(ext, ".jpg");
            img_data = stbi_load(jpg_path, &width, &height, &channels, 4);
        }
    }
    
    // If still not found, try default.png fallback
    if (!img_data && system_dir[0] != '\0') {
        char default_path[1024];
        snprintf(default_path, sizeof(default_path), 
                 "%s\\freeintvds-overlays\\default.png", system_dir);
        img_data = stbi_load(default_path, &width, &height, &channels, 4);
        
        // Try default.jpg if PNG not found
        if (!img_data) {
            snprintf(default_path, sizeof(default_path), 
                     "%s\\freeintvds-overlays\\default.jpg", system_dir);
            img_data = stbi_load(default_path, &width, &height, &channels, 4);
        }
    }
    
    if (img_data) {
        printf("[OVERLAY] Loaded overlay image: %dx%d from %s\n", width, height, overlay_path);
        
        // Store actual dimensions
        overlay_width = width;
        overlay_height = height;
        
        // Allocate overlay buffer at native size
        overlay_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            // Copy overlay at native resolution (no scaling)
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    
                    // Store as ARGB with original alpha channel
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                        overlay_buffer[y * width + x] =
                            (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            
            printf("[OVERLAY] Overlay stored at native %dx%d resolution\n", overlay_width, overlay_height);
                // Initialize overlay hotspots for touch
                init_overlay_hotspots(overlay_width, overlay_height);
                // Debug: render hotspot rectangles for layout check
                debug_render_hotspots(overlay_buffer, overlay_width, overlay_height);
        }
        
        stbi_image_free(img_data);
    } else {
        // Fallback: allocate and create test pattern at default overlay size
        overlay_width = 370;
        overlay_height = 600;
        overlay_buffer = (unsigned int*)malloc(overlay_width * overlay_height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            // 4-quadrant test pattern
            for (int y = 0; y < overlay_height; y++) {
                for (int x = 0; x < overlay_width; x++) {
                    if (y < overlay_height / 2 && x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF0000FF; // Blue in BGR
                    else if (y < overlay_height / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF00FF00; // Green
                    else if (x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFFFF0000; // Red in BGR  
                    else
                        overlay_buffer[y * overlay_width + x] = 0xFFFFFFFF; // White
                }
            }
        }
    }
    
    overlay_loaded = 1;
    strncpy(current_rom_path, rom_path, sizeof(current_rom_path) - 1);
}

// Safe dual-screen function using proven patterns
static void render_dual_screen(void)
{
    if (!dual_screen_enabled) return;
    // Allocate workspace buffer
    if (!dual_screen_buffer) {
        dual_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
    }
    if (!dual_screen_buffer) return;
    unsigned int* dual_buffer = (unsigned int*)dual_screen_buffer;
    // Defensive: check frame pointer
    extern unsigned int frame[352 * 224];
    int frame_valid = (frame != NULL);
    int overlay_valid = (overlay_buffer != NULL);
    // --- GAME SCREEN (Top: 704x448, scaled 2x from 352x224) ---
    for (int y = 0; y < 448; ++y) {
        int src_y = y / 2;  // Map to source row (0-223)
        for (int x = 0; x < 704; ++x) {
            int src_x = x / 2;  // Map to source column (0-351)
            if (frame_valid && src_y < GAME_HEIGHT && src_x < GAME_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = frame[src_y * GAME_WIDTH + src_x];
            } else {
                dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000; // Black
            }
        }
    }
    // --- CONTROLLER OVERLAY (Bottom: 704x620, layered) ---
    // Layer 1 (back): Game-specific overlay (centered under controller base, top-aligned, 1:1 pixels)
    // Layer 2 (front): Static controller base (right-aligned, native resolution, 1:1 pixels)
    
    // Controller base aligns to right edge of workspace
    int ctrl_base_x_start = WORKSPACE_WIDTH - controller_base_width;  // Right-aligned
    
    // Game overlay centered horizontally within controller base region
    int game_overlay_x_offset = (controller_base_width - overlay_width) / 2;
    
    for (int y = 0; y < OVERLAY_HEIGHT; ++y) {
        for (int x = 0; x < WORKSPACE_WIDTH; ++x) {
            unsigned int pixel = 0xFF000000; // Default black
            
            if (x >= ctrl_base_x_start) {
                // Controller base region: right side, aligned to right edge
                int ctrl_x = x - ctrl_base_x_start;  // Position within controller base
                
                // First render game overlay underneath (back layer, centered, top-aligned, 1:1 pixels)
                if (overlay_valid && y < overlay_height) {
                    int overlay_x_in_region = ctrl_x - game_overlay_x_offset;
                    
                    // Check if we're within the game overlay bounds
                    if (overlay_x_in_region >= 0 && overlay_x_in_region < overlay_width) {
                        // Direct 1:1 pixel mapping (no scaling)
                        unsigned int overlay_pixel = overlay_buffer[y * overlay_width + overlay_x_in_region];
                        unsigned int overlay_alpha = (overlay_pixel >> 24) & 0xFF;
                        // Render game overlay pixel
                        if (overlay_alpha > 0) {
                            pixel = overlay_pixel;
                        }
                    }
                }
                
                // Then layer controller base on top (front layer, 1:1 pixels)
                if (controller_base_loaded && controller_base && y < controller_base_height && ctrl_x < controller_base_width) {
                    // Direct 1:1 pixel mapping at native resolution
                    unsigned int base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    unsigned int alpha = (base_pixel >> 24) & 0xFF;
                    // Only draw controller base if not transparent (so game overlay shows through)
                    if (alpha > 0) {
                        pixel = base_pixel;
                    }
                }
            }
            // Left side remains black
            
            dual_buffer[(GAME_SCREEN_HEIGHT + y) * WORKSPACE_WIDTH + x] = pixel;
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
	
	// Store system directory for overlay path building
	if (SystemPath) {
		strncpy(system_dir, SystemPath, sizeof(system_dir) - 1);
		system_dir[sizeof(system_dir) - 1] = '\0';
		
		// Load static controller base image once at startup
		load_controller_base();
	}

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
	
	// Load overlay using the path from info
	if (dual_screen_enabled && info && info->path) {
		load_overlay_for_rom(info->path);
	}
	
	return true;
}

void retro_unload_game(void)
{
	quit(0);
}

// Button label rendering for overlay
// Intellivision keypad layout: 1 2 3 / 4 5 6 / 7 8 9 / C 0 E
typedef struct {
    int x, y, width, height;
    const char* label;
    unsigned int button_id;  // For future input mapping
} ButtonDef;

// Define 12-button Intellivision keypad layout (256x224 overlay area)
static ButtonDef intellivision_buttons[] = {
    // Row 1: 1, 2, 3
    {20, 20, 65, 45, "1", 0},
    {95, 20, 65, 45, "2", 0},
    {170, 20, 65, 45, "3", 0},
    
    // Row 2: 4, 5, 6
    {20, 70, 65, 45, "4", 0},
    {95, 70, 65, 45, "5", 0},
    {170, 70, 65, 45, "6", 0},
    
    // Row 3: 7, 8, 9
    {20, 120, 65, 45, "7", 0},
    {95, 120, 65, 45, "8", 0},
    {170, 120, 65, 45, "9", 0},
    
    // Row 4: Clear, 0, Enter
    {20, 170, 65, 45, "CLR", 0},
    {95, 170, 65, 45, "0", 0},
    {170, 170, 65, 45, "ENT", 0}
};

// Simple 5x7 bitmap font for digits and letters (minimal set)
static const unsigned char font_5x7[][7] = {
    // '0'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    // '1'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // '2'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
    // '3'
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
    // '4'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    // '5'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    // '6'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    // '7'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    // '8'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    // '9'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    // 'C'
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    // 'L'
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    // 'R'
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    // 'E'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    // 'N'
    {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11},
    // 'T'
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}
};

// Draw a single character at position (scale 2x)
static void draw_char(unsigned int* buffer, int buf_width, int x, int y, char c, unsigned int color)
{
    int char_index = -1;
    
    // Map character to font index
    if (c >= '0' && c <= '9') {
        char_index = c - '0';
    } else if (c == 'C') {
        char_index = 10;
    } else if (c == 'L') {
        char_index = 11;
    } else if (c == 'R') {
        char_index = 12;
    } else if (c == 'E') {
        char_index = 13;
    } else if (c == 'N') {
        char_index = 14;
    } else if (c == 'T') {
        char_index = 15;
    } else {
        return;  // Unsupported character
    }
    
    // Draw 5x7 character scaled 2x (10x14 pixels)
    for (int row = 0; row < 7; row++) {
        unsigned char line = font_5x7[char_index][row];
        for (int col = 0; col < 5; col++) {
            if (line & (1 << (4 - col))) {
                // Draw 2x2 pixel block
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int px = x + col * 2 + dx;
                        int py = y + row * 2 + dy;
                        if (px >= 0 && px < buf_width && py >= 0 && py < GAME_HEIGHT) {
                            buffer[py * buf_width + px] = color;
                        }
                    }
                }
            }
        }
    }
}

// Draw text string centered in button
static void draw_text_centered(unsigned int* buffer, int buf_width, int center_x, int center_y, 
                               const char* text, unsigned int color)
{
    int len = strlen(text);
    int char_width = 10;  // 5 pixels * 2 scale
    int char_spacing = 2;
    int total_width = len * char_width + (len - 1) * char_spacing;
    int start_x = center_x - total_width / 2;
    int start_y = center_y - 7;  // 7 rows * 2 scale / 2
    
    for (int i = 0; i < len; i++) {
        draw_char(buffer, buf_width, start_x + i * (char_width + char_spacing), start_y, text[i], color);
    }
}

// Draw button outline (semi-transparent border)
static void draw_button_outline(unsigned int* buffer, int buf_width, int x, int y, int width, int height, 
                                unsigned int color, int thickness)
{
    // Top and bottom borders
    for (int dx = 0; dx < width; dx++) {
        for (int t = 0; t < thickness; t++) {
            int px = x + dx;
            // Top border
            if (px >= 0 && px < buf_width && (y + t) >= 0 && (y + t) < GAME_HEIGHT) {
                buffer[(y + t) * buf_width + px] = color;
            }
            // Bottom border
            if (px >= 0 && px < buf_width && (y + height - 1 - t) >= 0 && (y + height - 1 - t) < GAME_HEIGHT) {
                buffer[(y + height - 1 - t) * buf_width + px] = color;
            }
        }
    }
    
    // Left and right borders
    for (int dy = 0; dy < height; dy++) {
        for (int t = 0; t < thickness; t++) {
            int py = y + dy;
            // Left border
            if ((x + t) >= 0 && (x + t) < buf_width && py >= 0 && py < GAME_HEIGHT) {
                buffer[py * buf_width + (x + t)] = color;
            }
            // Right border
            if ((x + width - 1 - t) >= 0 && (x + width - 1 - t) < buf_width && py >= 0 && py < GAME_HEIGHT) {
                buffer[py * buf_width + (x + width - 1 - t)] = color;
            }
        }
    }
}

// Render all button labels on overlay
static void render_button_labels(unsigned int* buffer, int buf_width)
{
    int num_buttons = sizeof(intellivision_buttons) / sizeof(ButtonDef);
    
    // Semi-transparent white for outlines (70% opacity)
    unsigned int outline_color = 0xB3FFFFFF;
    
    // Solid white for text
    unsigned int text_color = 0xFFFFFFFF;
    
    for (int i = 0; i < num_buttons; i++) {
        ButtonDef* btn = &intellivision_buttons[i];
        
        // Draw button outline
        draw_button_outline(buffer, buf_width, btn->x, btn->y, btn->width, btn->height, 
                          outline_color, 2);
        
        // Draw button label centered
        int center_x = btn->x + btn->width / 2;
        int center_y = btn->y + btn->height / 2;
        draw_text_centered(buffer, buf_width, center_x, center_y, btn->label, text_color);
    }
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
	
	// Send frame to libretro - use dual-screen buffer if enabled
	if (dual_screen_enabled) {
		// Update dual-screen buffer AFTER Run() updates the game frame
		render_dual_screen();
		
		// Only send dual buffer if it was successfully allocated
		if (dual_screen_buffer) {
			Video(dual_screen_buffer, WORKSPACE_WIDTH, WORKSPACE_HEIGHT, sizeof(unsigned int) * WORKSPACE_WIDTH);
		} else {
			// Fallback to regular single screen if allocation failed
			Video(frame, frameWidth, frameHeight, sizeof(unsigned int) * frameWidth);
		}
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
    int width = dual_screen_enabled ? WORKSPACE_WIDTH : GAME_WIDTH;
    int height = dual_screen_enabled ? WORKSPACE_HEIGHT : GAME_HEIGHT;
    memset(info, 0, sizeof(*info));
    info->geometry.base_width   = width;
    info->geometry.base_height  = height;
    info->geometry.max_width    = width;
    info->geometry.max_height   = height;
    // Use actual workspace aspect ratio so frontend doesn't stretch
    info->geometry.aspect_ratio = ((float)width) / ((float)height);
    info->timing.fps = DefaultFPS;
    info->timing.sample_rate = AUDIO_FREQUENCY;
    Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void)
{
	libretro_supports_option_categories = false;
	
	// Clean up dual-screen buffers
	if (dual_screen_buffer) {
		free(dual_screen_buffer);
		dual_screen_buffer = NULL;
	}
	
	if (overlay_buffer) {
		free(overlay_buffer);
		overlay_buffer = NULL;
	}
	
	if (controller_base) {
		free(controller_base);
		controller_base = NULL;
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
