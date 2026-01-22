#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "retropac.h"

#define MODE_DEFAULT "default"

/* Extract ROM name from file path */
char *extract_rom_name(const char *rom_path) {
    if (!rom_path) return NULL;
    
    /* Make a copy of the path for basename() which may modify it */
    char *path_copy = strdup(rom_path);
    if (!path_copy) return NULL;
    
    /* Get the basename (filename with extension) */
    char *base = basename(path_copy);
    char *rom_name = strdup(base);
    
    free(path_copy);
    
    if (!rom_name) return NULL;
    
    /* Remove file extension */
    char *dot = strrchr(rom_name, '.');
    if (dot) {
        *dot = '\0';
    }
    
    return rom_name;
}

/* Find ROM configuration for given emulator and ROM */
static RomConfig *find_rom_config(Config *config, const char *emulator_name, const char *rom_name) {
    if (!config || !emulator_name || !rom_name) return NULL;
    
    /* Find emulator */
    EmulatorConfig *emulator = NULL;
    for (int i = 0; i < config->emulator_count; i++) {
        if (strcmp(config->emulators[i].emulator_name, emulator_name) == 0) {
            emulator = &config->emulators[i];
            break;
        }
    }
    
    if (!emulator) {
        fprintf(stderr, "Warning: Emulator '%s' not found in config\n", emulator_name);
        return NULL;
    }
    
    /* Find ROM */
    RomConfig *rom = NULL;
    for (int i = 0; i < emulator->rom_count; i++) {
        if (strcmp(emulator->roms[i].rom_name, rom_name) == 0) {
            rom = &emulator->roms[i];
            break;
        }
    }
    
    if (!rom) {
        /* Try to find "default" ROM config */
        for (int i = 0; i < emulator->rom_count; i++) {
            if (strcmp(emulator->roms[i].rom_name, "default") == 0) {
                rom = &emulator->roms[i];
                printf("Using default configuration for emulator '%s'\n", emulator_name);
                break;
            }
        }
    }
    
    if (!rom) {
        fprintf(stderr, "Warning: ROM '%s' not found in emulator '%s' config and no default exists\n",
                rom_name, emulator_name);
    }
    
    return rom;
}

int main(int argc, char *argv[]) {
    const char *config_file = "/home/pi/RetroPie/configs/retropac/config.json";
    const char *emulator_name;
    const char *rom_path;
    const char *mode = NULL;
    char *rom_name = NULL;
    Config *config = NULL;
    RomConfig *rom_config = NULL;
    int ipac_handle = -1;
    int exit_code = 0;
    
    printf("RetroPac - Ultimarc i-pac LED Controller v1.0\n");
    printf("==============================================\n\n");
    
    /* Check arguments */
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <emulator> <rom_path> [mode]\n", argv[0]);
        fprintf(stderr, "  Run with specific game:\n");
        fprintf(stderr, "    %s mame /home/pi/RetroPie/roms/mame/sf2.zip\n", argv[0]);
        fprintf(stderr, "  Run with default config (for EmulationStation menu):\n");
        fprintf(stderr, "    %s default default default\n", argv[0]);
        return 1;
    }
    
    emulator_name = argv[1];
    rom_path = argv[2];
    
    printf("Emulator: %s\n", emulator_name);
    printf("ROM path: %s\n", rom_path);
    
    /* Check if we're in default mode first */
    if (argc >= 4 && strcmp(argv[3], MODE_DEFAULT) == 0) {
        mode = MODE_DEFAULT;
        printf("Mode: %s\n\n", mode);
    } else {
        /* Extract ROM name from path (only needed for game-specific configs) */
        rom_name = extract_rom_name(rom_path);
        if (!rom_name) {
            fprintf(stderr, "Error: Could not extract ROM name from path\n");
            exit_code = 1;
            goto cleanup;
        }
        printf("ROM name: %s\n\n", rom_name);
    }
    
    /* Load configuration */
    printf("Loading configuration from %s...\n", config_file);
    config = load_config(config_file);
    if (!config) {
        fprintf(stderr, "Error: Could not load configuration\n");
        exit_code = 1;
        goto cleanup;
    }
    printf("Configuration loaded successfully\n");
    printf("  Controllers: %d\n", config->controller_count);
    printf("  Emulators: %d\n", config->emulator_count);
    printf("  Default config: %s\n\n", config->default_config ? "Yes" : "No");
    
    /* Find ROM configuration */
    /* If mode is "default", use the top-level default configuration */
    if (mode && strcmp(mode, MODE_DEFAULT) == 0) {
        if (config->default_config) {
            rom_config = config->default_config;
            printf("Using top-level default configuration\n\n");
        } else {
            fprintf(stderr, "Error: No top-level default configuration found\n");
            exit_code = 1;
            goto cleanup;
        }
    } else {
        rom_config = find_rom_config(config, emulator_name, rom_name);
        if (!rom_config) {
            fprintf(stderr, "Error: Could not find configuration for this ROM\n");
            exit_code = 1;
            goto cleanup;
        }
    }
    printf("Found ROM configuration with %d buttons\n\n", rom_config->button_count);
    
    /* Initialize i-pac controller */
    if (config->controller_count > 0) {
        printf("Initializing i-pac controller...\n");
        ipac_handle = ipac_init(&config->controllers[0]);
        if (ipac_handle < 0) {
            fprintf(stderr, "Warning: Could not initialize i-pac controller\n");
            fprintf(stderr, "Continuing in simulation mode (no hardware control)\n\n");
            ipac_handle = -1;
        } else {
            printf("\n");
        }
    } else {
        fprintf(stderr, "Warning: No i-pac controllers defined in configuration\n");
        fprintf(stderr, "Running in simulation mode\n\n");
    }
    
    /* Set LEDs */
    if (ipac_handle >= 0) {
        if (ipac_set_all_leds(ipac_handle, rom_config->buttons, rom_config->button_count, config->controllers[0].pin_mappings) < 0) {
            fprintf(stderr, "Warning: Some LEDs could not be set\n");
        }
    } else {
        /* Simulation mode - just print what would be set */
        printf("Simulation mode - would set the following LEDs:\n");
        for (int i = 0; i < rom_config->button_count; i++) {
            printf("  %s -> RGB(%d, %d, %d)\n",
                   button_enum_to_name(rom_config->buttons[i].button),
                   rom_config->buttons[i].color.r,
                   rom_config->buttons[i].color.g,
                   rom_config->buttons[i].color.b);
        }
    }
    
    printf("\nRetroPac completed successfully\n");
    
cleanup:
    if (ipac_handle >= 0) {
        ipac_close(ipac_handle);
    }
    if (rom_name) {
        free(rom_name);
    }
    if (config) {
        free_config(config);
    }
    
    return exit_code;
}
