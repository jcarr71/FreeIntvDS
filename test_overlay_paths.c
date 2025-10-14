#include <stdio.h>
#include <string.h>

// Test function to demonstrate overlay path building
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
    
    // Build path: ROM_DIR/overlays/ROM_NAME.png
    snprintf(overlay_path, overlay_path_size, "%.*soverlays/%.*s.png", 
             (int)dir_len, rom_path, (int)name_len, filename);
}

int main() {
    char overlay_path[1024];
    
    // Test different ROM path formats
    printf("=== Overlay Path Examples ===\n\n");
    
    // Your specific case
    build_overlay_path("c:\\Roms\\Astrosmash (USA, Europe).zip", overlay_path, sizeof(overlay_path));
    printf("ROM: c:\\Roms\\Astrosmash (USA, Europe).zip\n");
    printf("Overlay: %s\n\n", overlay_path);
    
    // Windows style path
    build_overlay_path("C:\\ROMs\\Intellivision\\Space Armada (USA).rom", overlay_path, sizeof(overlay_path));
    printf("ROM: C:\\ROMs\\Intellivision\\Space Armada (USA).rom\n");
    printf("Overlay: %s\n\n", overlay_path);
    
    // Unix style path  
    build_overlay_path("/home/user/roms/Astrosmash (USA).bin", overlay_path, sizeof(overlay_path));
    printf("ROM: /home/user/roms/Astrosmash (USA).bin\n");
    printf("Overlay: %s\n\n", overlay_path);
    
    return 0;
}