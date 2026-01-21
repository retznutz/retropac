#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "retropac.h"

/* Convert button name string to ButtonType enum */
ButtonType button_name_to_enum(const char *name) {
    if (strcmp(name, "P1_COIN") == 0) return P1_COIN;
    if (strcmp(name, "P2_COIN") == 0) return P2_COIN;
    if (strcmp(name, "P3_COIN") == 0) return P3_COIN;
    if (strcmp(name, "P4_COIN") == 0) return P4_COIN;
    
    if (strcmp(name, "P1_START") == 0) return P1_START;
    if (strcmp(name, "P2_START") == 0) return P2_START;
    if (strcmp(name, "P3_START") == 0) return P3_START;
    if (strcmp(name, "P4_START") == 0) return P4_START;
    
    if (strcmp(name, "P1_BUTTON1") == 0) return P1_BUTTON1;
    if (strcmp(name, "P1_BUTTON2") == 0) return P1_BUTTON2;
    if (strcmp(name, "P1_BUTTON3") == 0) return P1_BUTTON3;
    if (strcmp(name, "P1_BUTTON4") == 0) return P1_BUTTON4;
    if (strcmp(name, "P1_BUTTON5") == 0) return P1_BUTTON5;
    if (strcmp(name, "P1_BUTTON6") == 0) return P1_BUTTON6;
    
    if (strcmp(name, "P2_BUTTON1") == 0) return P2_BUTTON1;
    if (strcmp(name, "P2_BUTTON2") == 0) return P2_BUTTON2;
    if (strcmp(name, "P2_BUTTON3") == 0) return P2_BUTTON3;
    if (strcmp(name, "P2_BUTTON4") == 0) return P2_BUTTON4;
    if (strcmp(name, "P2_BUTTON5") == 0) return P2_BUTTON5;
    if (strcmp(name, "P2_BUTTON6") == 0) return P2_BUTTON6;
    
    if (strcmp(name, "P3_BUTTON1") == 0) return P3_BUTTON1;
    if (strcmp(name, "P3_BUTTON2") == 0) return P3_BUTTON2;
    if (strcmp(name, "P3_BUTTON3") == 0) return P3_BUTTON3;
    if (strcmp(name, "P3_BUTTON4") == 0) return P3_BUTTON4;
    if (strcmp(name, "P3_BUTTON5") == 0) return P3_BUTTON5;
    if (strcmp(name, "P3_BUTTON6") == 0) return P3_BUTTON6;
    
    if (strcmp(name, "P4_BUTTON1") == 0) return P4_BUTTON1;
    if (strcmp(name, "P4_BUTTON2") == 0) return P4_BUTTON2;
    if (strcmp(name, "P4_BUTTON3") == 0) return P4_BUTTON3;
    if (strcmp(name, "P4_BUTTON4") == 0) return P4_BUTTON4;
    if (strcmp(name, "P4_BUTTON5") == 0) return P4_BUTTON5;
    if (strcmp(name, "P4_BUTTON6") == 0) return P4_BUTTON6;
    
    if (strcmp(name, "P1_JOYSTICK") == 0) return P1_JOYSTICK;
    if (strcmp(name, "P2_JOYSTICK") == 0) return P2_JOYSTICK;
    if (strcmp(name, "P3_JOYSTICK") == 0) return P3_JOYSTICK;
    if (strcmp(name, "P4_JOYSTICK") == 0) return P4_JOYSTICK;
    
    if (strcmp(name, "P1_TRACKBALL") == 0) return P1_TRACKBALL;
    if (strcmp(name, "P2_TRACKBALL") == 0) return P2_TRACKBALL;
    if (strcmp(name, "P3_TRACKBALL") == 0) return P3_TRACKBALL;
    if (strcmp(name, "P4_TRACKBALL") == 0) return P4_TRACKBALL;
    
    return BUTTON_MAX; /* Invalid button */
}

/* Convert ButtonType enum to string name */
const char *button_enum_to_name(ButtonType button) {
    static const char *button_names[] = {
        "P1_COIN", "P2_COIN", "P3_COIN", "P4_COIN",
        "P1_START", "P2_START", "P3_START", "P4_START",
        "P1_BUTTON1", "P1_BUTTON2", "P1_BUTTON3", "P1_BUTTON4", "P1_BUTTON5", "P1_BUTTON6",
        "P2_BUTTON1", "P2_BUTTON2", "P2_BUTTON3", "P2_BUTTON4", "P2_BUTTON5", "P2_BUTTON6",
        "P3_BUTTON1", "P3_BUTTON2", "P3_BUTTON3", "P3_BUTTON4", "P3_BUTTON5", "P3_BUTTON6",
        "P4_BUTTON1", "P4_BUTTON2", "P4_BUTTON3", "P4_BUTTON4", "P4_BUTTON5", "P4_BUTTON6",
        "P1_JOYSTICK", "P2_JOYSTICK", "P3_JOYSTICK", "P4_JOYSTICK",
        "P1_TRACKBALL", "P2_TRACKBALL", "P3_TRACKBALL", "P4_TRACKBALL"
    };
    
    if (button >= 0 && button < BUTTON_MAX) {
        return button_names[button];
    }
    return "UNKNOWN";
}

/* Parse RGB color from JSON object */
static int parse_color(struct json_object *color_obj, RGBColor *color) {
    struct json_object *r_obj, *g_obj, *b_obj;
    
    if (!json_object_object_get_ex(color_obj, "r", &r_obj) ||
        !json_object_object_get_ex(color_obj, "g", &g_obj) ||
        !json_object_object_get_ex(color_obj, "b", &b_obj)) {
        return -1;
    }
    
    color->r = (uint8_t)json_object_get_int(r_obj);
    color->g = (uint8_t)json_object_get_int(g_obj);
    color->b = (uint8_t)json_object_get_int(b_obj);
    
    return 0;
}

/* Parse ROM configuration from JSON */
static RomConfig *parse_rom(const char *rom_name, struct json_object *rom_obj) {
    RomConfig *rom = malloc(sizeof(RomConfig));
    if (!rom) return NULL;
    
    rom->rom_name = strdup(rom_name);
    rom->button_count = 0;
    
    /* Count buttons */
    json_object_object_foreach(rom_obj, key, val) {
        rom->button_count++;
    }
    
    /* Allocate button array */
    rom->buttons = malloc(sizeof(ButtonConfig) * rom->button_count);
    if (!rom->buttons) {
        free(rom->rom_name);
        free(rom);
        return NULL;
    }
    
    /* Parse buttons */
    int i = 0;
    json_object_object_foreach(rom_obj, key, val) {
        ButtonType button_type = button_name_to_enum(key);
        if (button_type == BUTTON_MAX) {
            fprintf(stderr, "Warning: Unknown button '%s'\n", key);
            continue;
        }
        
        rom->buttons[i].button = button_type;
        if (parse_color(val, &rom->buttons[i].color) < 0) {
            fprintf(stderr, "Warning: Invalid color for button '%s'\n", key);
            continue;
        }
        i++;
    }
    rom->button_count = i;
    
    return rom;
}

/* Parse emulator configuration from JSON */
static EmulatorConfig *parse_emulator(const char *emu_name, struct json_object *emu_obj) {
    EmulatorConfig *emulator = malloc(sizeof(EmulatorConfig));
    if (!emulator) return NULL;
    
    emulator->emulator_name = strdup(emu_name);
    emulator->rom_count = 0;
    
    struct json_object *roms_obj;
    if (!json_object_object_get_ex(emu_obj, "roms", &roms_obj)) {
        free(emulator->emulator_name);
        free(emulator);
        return NULL;
    }
    
    /* Count ROMs */
    json_object_object_foreach(roms_obj, key, val) {
        emulator->rom_count++;
    }
    
    /* Allocate ROM array */
    emulator->roms = malloc(sizeof(RomConfig) * emulator->rom_count);
    if (!emulator->roms) {
        free(emulator->emulator_name);
        free(emulator);
        return NULL;
    }
    
    /* Parse ROMs */
    int i = 0;
    json_object_object_foreach(roms_obj, key, val) {
        RomConfig *rom = parse_rom(key, val);
        if (rom) {
            emulator->roms[i] = *rom;
            free(rom);
            i++;
        }
    }
    emulator->rom_count = i;
    
    return emulator;
}

/* Parse i-pac controller from JSON */
static IpacController *parse_controller(struct json_object *ctrl_obj) {
    IpacController *controller = malloc(sizeof(IpacController));
    if (!controller) return NULL;
    
    struct json_object *device_obj, *vendor_obj, *product_obj;
    
    if (json_object_object_get_ex(ctrl_obj, "device", &device_obj)) {
        controller->device_name = strdup(json_object_get_string(device_obj));
    } else {
        controller->device_name = strdup("unknown");
    }
    
    if (json_object_object_get_ex(ctrl_obj, "vendor_id", &vendor_obj)) {
        const char *vendor_str = json_object_get_string(vendor_obj);
        controller->vendor_id = (uint16_t)strtol(vendor_str, NULL, 16);
    } else {
        controller->vendor_id = 0xd208; /* Default Ultimarc vendor ID */
    }
    
    if (json_object_object_get_ex(ctrl_obj, "product_id", &product_obj)) {
        const char *product_str = json_object_get_string(product_obj);
        controller->product_id = (uint16_t)strtol(product_str, NULL, 16);
    } else {
        controller->product_id = 0x0310; /* Default i-pac Ultimate I/O */
    }
    
    return controller;
}

/* Load configuration from JSON file */
Config *load_config(const char *filename) {
    struct json_object *root;
    Config *config = NULL;
    
    root = json_object_from_file(filename);
    if (!root) {
        fprintf(stderr, "Error: Could not load config file '%s'\n", filename);
        return NULL;
    }
    
    config = malloc(sizeof(Config));
    if (!config) {
        json_object_put(root);
        return NULL;
    }
    
    config->controller_count = 0;
    config->emulator_count = 0;
    config->controllers = NULL;
    config->emulators = NULL;
    config->default_config = NULL;
    
    /* Parse i-pac controllers */
    struct json_object *controllers_obj;
    if (json_object_object_get_ex(root, "ipac_controllers", &controllers_obj)) {
        int array_len = json_object_array_length(controllers_obj);
        config->controllers = malloc(sizeof(IpacController) * array_len);
        
        for (int i = 0; i < array_len; i++) {
            struct json_object *ctrl_obj = json_object_array_get_idx(controllers_obj, i);
            IpacController *controller = parse_controller(ctrl_obj);
            if (controller) {
                config->controllers[config->controller_count] = *controller;
                free(controller);
                config->controller_count++;
            }
        }
    }
    
    /* Parse top-level default configuration */
    struct json_object *default_obj;
    if (json_object_object_get_ex(root, "default", &default_obj)) {
        config->default_config = parse_rom("default", default_obj);
        if (!config->default_config) {
            fprintf(stderr, "Warning: Failed to parse top-level default configuration\n");
        }
    }
    
    /* Parse emulators */
    struct json_object *emulators_obj;
    if (json_object_object_get_ex(root, "emulators", &emulators_obj)) {
        /* Count emulators */
        json_object_object_foreach(emulators_obj, key, val) {
            config->emulator_count++;
        }
        
        /* Allocate emulator array */
        config->emulators = malloc(sizeof(EmulatorConfig) * config->emulator_count);
        
        /* Parse emulators */
        int i = 0;
        json_object_object_foreach(emulators_obj, key, val) {
            EmulatorConfig *emulator = parse_emulator(key, val);
            if (emulator) {
                config->emulators[i] = *emulator;
                free(emulator);
                i++;
            }
        }
        config->emulator_count = i;
    }
    
    json_object_put(root);
    return config;
}

/* Free configuration structure */
void free_config(Config *config) {
    if (!config) return;
    
    /* Free controllers */
    if (config->controllers) {
        for (int i = 0; i < config->controller_count; i++) {
            free(config->controllers[i].device_name);
        }
        free(config->controllers);
    }
    
    /* Free default configuration */
    if (config->default_config) {
        free(config->default_config->rom_name);
        free(config->default_config->buttons);
        free(config->default_config);
    }
    
    /* Free emulators */
    if (config->emulators) {
        for (int i = 0; i < config->emulator_count; i++) {
            EmulatorConfig *emulator = &config->emulators[i];
            free(emulator->emulator_name);
            
            if (emulator->roms) {
                for (int j = 0; j < emulator->rom_count; j++) {
                    free(emulator->roms[j].rom_name);
                    free(emulator->roms[j].buttons);
                }
                free(emulator->roms);
            }
        }
        free(config->emulators);
    }
    
    free(config);
}
