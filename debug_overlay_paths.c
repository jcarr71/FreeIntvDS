#include <stdio.h>
#include <string.h>

// Same function as in libretro.c
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0) return;
    
    // Find the last slash or backslash to get filename
    const char* filename = rom_path;
    const char* p = rom_path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    // Find the directory part (everything before the filename)
    size_t dir_len = filename - rom_path;
    
    // Find the extension and remove it
    const char* ext = filename;
    const char* q = filename;
    while (*q) {
        if (*q == '.') {
            ext = q;
        }
        q++;
    }
    
    // Calculate filename without extension length
    size_t name_len = ext - filename;
    
    // Build path: ROM_DIR\overlays\ROM_NAME.png (Windows style)
    snprintf(overlay_path, overlay_path_size, "%.*soverlays\\%.*s.png", 
             (int)dir_len, rom_path, (int)name_len, filename);
}

int main() {
    char overlay_path[1024];
    char overlay_path_caps[1024];
    
    // Test your exact file
    const char* rom_path = "c:\\Roms\\Astrosmash (USA, Europe).zip";
    
    printf("=== Testing Your Specific Files ===\n\n");
    printf("ROM: %s\n\n", rom_path);
    
    // Test lowercase
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    printf("Trying: %s\n", overlay_path);
    FILE* file = fopen(overlay_path, "rb");
    if (file) {
        printf("✓ FOUND!\n");
        fclose(file);
    } else {
        printf("✗ Not found\n");
    }
    
    // Test uppercase
    build_overlay_path(rom_path, overlay_path_caps, sizeof(overlay_path_caps));
    char* overlays_pos = strstr(overlay_path_caps, "overlays\\");
    if (overlays_pos) {
        memcpy(overlays_pos, "Overlays\\", 9);
        printf("Trying: %s\n", overlay_path_caps);
        file = fopen(overlay_path_caps, "rb");
        if (file) {
            printf("✓ FOUND!\n");
            fclose(file);
        } else {
            printf("✗ Not found\n");
        }
    }
    
    // Let's also check if the paths exist
    printf("\nPlease verify these files exist:\n");
    printf("1. %s\n", overlay_path);
    printf("2. %s\n", overlay_path_caps);
    
    return 0;
}