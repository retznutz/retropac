#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
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
    if (strcmp(name, "P1_BUTTON7") == 0) return P1_BUTTON7;
    if (strcmp(name, "P1_BUTTON8") == 0) return P1_BUTTON8;
    
    if (strcmp(name, "P2_BUTTON1") == 0) return P2_BUTTON1;
    if (strcmp(name, "P2_BUTTON2") == 0) return P2_BUTTON2;
    if (strcmp(name, "P2_BUTTON3") == 0) return P2_BUTTON3;
    if (strcmp(name, "P2_BUTTON4") == 0) return P2_BUTTON4;
    if (strcmp(name, "P2_BUTTON5") == 0) return P2_BUTTON5;
    if (strcmp(name, "P2_BUTTON6") == 0) return P2_BUTTON6;
    if (strcmp(name, "P2_BUTTON7") == 0) return P2_BUTTON7;
    if (strcmp(name, "P2_BUTTON8") == 0) return P2_BUTTON8;
    
    if (strcmp(name, "P3_BUTTON1") == 0) return P3_BUTTON1;
    if (strcmp(name, "P3_BUTTON2") == 0) return P3_BUTTON2;
    if (strcmp(name, "P3_BUTTON3") == 0) return P3_BUTTON3;
    if (strcmp(name, "P3_BUTTON4") == 0) return P3_BUTTON4;
    if (strcmp(name, "P3_BUTTON5") == 0) return P3_BUTTON5;
    if (strcmp(name, "P3_BUTTON6") == 0) return P3_BUTTON6;
    if (strcmp(name, "P3_BUTTON7") == 0) return P3_BUTTON7;
    if (strcmp(name, "P3_BUTTON8") == 0) return P3_BUTTON8;
    
    if (strcmp(name, "P4_BUTTON1") == 0) return P4_BUTTON1;
    if (strcmp(name, "P4_BUTTON2") == 0) return P4_BUTTON2;
    if (strcmp(name, "P4_BUTTON3") == 0) return P4_BUTTON3;
    if (strcmp(name, "P4_BUTTON4") == 0) return P4_BUTTON4;
    if (strcmp(name, "P4_BUTTON5") == 0) return P4_BUTTON5;
    if (strcmp(name, "P4_BUTTON6") == 0) return P4_BUTTON6;
    if (strcmp(name, "P4_BUTTON7") == 0) return P4_BUTTON7;
    if (strcmp(name, "P4_BUTTON8") == 0) return P4_BUTTON8;
    
    if (strcmp(name, "P1_JOYSTICK") == 0) return P1_JOYSTICK;
    if (strcmp(name, "P2_JOYSTICK") == 0) return P2_JOYSTICK;
    if (strcmp(name, "P3_JOYSTICK") == 0) return P3_JOYSTICK;
    if (strcmp(name, "P4_JOYSTICK") == 0) return P4_JOYSTICK;
    
    if (strcmp(name, "P1_TRACKBALL") == 0) return P1_TRACKBALL;
    if (strcmp(name, "P2_TRACKBALL") == 0) return P2_TRACKBALL;
    if (strcmp(name, "P3_TRACKBALL") == 0) return P3_TRACKBALL;
    if (strcmp(name, "P4_TRACKBALL") == 0) return P4_TRACKBALL;
    
    if (strcmp(name, "P1_LIGHTGUN") == 0) return P1_LIGHTGUN;
    if (strcmp(name, "P2_LIGHTGUN") == 0) return P2_LIGHTGUN;
    
    if (strcmp(name, "P1_DIAL") == 0) return P1_DIAL;
    if (strcmp(name, "P2_DIAL") == 0) return P2_DIAL;
    
    if (strcmp(name, "P1_PADDLE") == 0) return P1_PADDLE;
    if (strcmp(name, "P2_PADDLE") == 0) return P2_PADDLE;
    
    if (strcmp(name, "P1_STICK") == 0) return P1_STICK;
    if (strcmp(name, "P2_STICK") == 0) return P2_STICK;
    
    return BUTTON_MAX; /* Invalid button */
}

/* Convert ButtonType enum to string name */
const char *button_enum_to_name(ButtonType button) {
    static const char *button_names[] = {
        "P1_COIN", "P2_COIN", "P3_COIN", "P4_COIN",
        "P1_START", "P2_START", "P3_START", "P4_START",
        "P1_BUTTON1", "P1_BUTTON2", "P1_BUTTON3", "P1_BUTTON4", "P1_BUTTON5", "P1_BUTTON6", "P1_BUTTON7", "P1_BUTTON8",
        "P2_BUTTON1", "P2_BUTTON2", "P2_BUTTON3", "P2_BUTTON4", "P2_BUTTON5", "P2_BUTTON6", "P2_BUTTON7", "P2_BUTTON8",
        "P3_BUTTON1", "P3_BUTTON2", "P3_BUTTON3", "P3_BUTTON4", "P3_BUTTON5", "P3_BUTTON6", "P3_BUTTON7", "P3_BUTTON8",
        "P4_BUTTON1", "P4_BUTTON2", "P4_BUTTON3", "P4_BUTTON4", "P4_BUTTON5", "P4_BUTTON6", "P4_BUTTON7", "P4_BUTTON8",
        "P1_JOYSTICK", "P2_JOYSTICK", "P3_JOYSTICK", "P4_JOYSTICK",
        "P1_TRACKBALL", "P2_TRACKBALL", "P3_TRACKBALL", "P4_TRACKBALL",
        "P1_LIGHTGUN", "P2_LIGHTGUN",
        "P1_DIAL", "P2_DIAL",
        "P1_PADDLE", "P2_PADDLE",
        "P1_STICK", "P2_STICK"
    };
    
    if (button >= 0 && button < BUTTON_MAX) {
        return button_names[button];
    }
    return "UNKNOWN";
}

/* Parse color from hex string (e.g., "#FF0000" or "FF0000") */
static int parse_color_string(const char *hex_str, RGBColor *color) {
    if (!hex_str || !color) return -1;
    
    /* Skip '#' if present */
    if (hex_str[0] == '#') {
        hex_str++;
    }
    
    /* Validate hex string length */
    if (strlen(hex_str) != 6) {
        return -1;
    }
    
    unsigned int hex_value;
    if (sscanf(hex_str, "%6x", &hex_value) != 1) {
        return -1;
    }
    
    color->r = (hex_value >> 16) & 0xFF;
    color->g = (hex_value >> 8) & 0xFF;
    color->b = hex_value & 0xFF;
    
    return 0;
}

/* Parse RGB color from JSON object containing hex string */
static int parse_color(struct json_object *color_obj, RGBColor *color) {
    if (json_object_get_type(color_obj) != json_type_string) {
        return -1;
    }
    
    const char *hex_str = json_object_get_string(color_obj);
    return parse_color_string(hex_str, color);
}

/* Parse controller-specific button configuration from JSON */
static ControllerButtonConfig *parse_controller_buttons(const char *controller_name, struct json_object *buttons_obj) {
    ControllerButtonConfig *ctrl_config = malloc(sizeof(ControllerButtonConfig));
    if (!ctrl_config) return NULL;
    
    ctrl_config->controller_name = strdup(controller_name);
    ctrl_config->button_count = 0;
    
    /* Count buttons */
    {
        json_object_object_foreach(buttons_obj, key, val) {
            (void)key; (void)val;
            ctrl_config->button_count++;
        }
    }
    
    /* Allocate button array */
    ctrl_config->buttons = malloc(sizeof(ButtonConfig) * ctrl_config->button_count);
    if (!ctrl_config->buttons) {
        free(ctrl_config->controller_name);
        free(ctrl_config);
        return NULL;
    }
    
    /* Parse buttons */
    int i = 0;
    {
        json_object_object_foreach(buttons_obj, key, val) {
            ButtonType button_type = button_name_to_enum(key);
            if (button_type == BUTTON_MAX) {
                fprintf(stderr, "Warning: Unknown button '%s' for controller '%s'\n", key, controller_name);
                continue;
            }
            
            ctrl_config->buttons[i].button = button_type;
            if (parse_color(val, &ctrl_config->buttons[i].color) < 0) {
                fprintf(stderr, "Warning: Invalid color for button '%s'\n", key);
                continue;
            }
            i++;
        }
    }
    ctrl_config->button_count = i;
    
    return ctrl_config;
}

/* Parse ROM configuration from JSON (new format with controllers) */
static RomConfig *parse_rom(const char *rom_name, struct json_object *rom_obj) {
    RomConfig *rom = malloc(sizeof(RomConfig));
    if (!rom) return NULL;
    
    rom->rom_name = strdup(rom_name);
    rom->controller_configs = NULL;
    rom->controller_config_count = 0;
    
    /* Look for "controllers" object in the new format */
    struct json_object *controllers_obj;
    if (json_object_object_get_ex(rom_obj, "controllers", &controllers_obj)) {
        /* New format: { "controllers": { "device_name": { buttons... }, ... } } */
        
        /* Count controllers */
        int ctrl_count = 0;
        {
            json_object_object_foreach(controllers_obj, key, val) {
                (void)key; (void)val;
                ctrl_count++;
            }
        }
        
        if (ctrl_count > 0) {
            rom->controller_configs = malloc(sizeof(ControllerButtonConfig) * ctrl_count);
            if (!rom->controller_configs) {
                free(rom->rom_name);
                free(rom);
                return NULL;
            }
            
            /* Parse each controller's button configuration */
            int i = 0;
            {
                json_object_object_foreach(controllers_obj, ctrl_name, buttons_obj) {
                    ControllerButtonConfig *ctrl_config = parse_controller_buttons(ctrl_name, buttons_obj);
                    if (ctrl_config) {
                        rom->controller_configs[i] = *ctrl_config;
                        free(ctrl_config);
                        i++;
                    }
                }
            }
            rom->controller_config_count = i;
        }
    } else {
        /* Legacy format: { "P1_BUTTON1": "#FF0000", ... } - treat as first controller */
        /* For backwards compatibility, parse buttons directly and assign to "default" controller */
        int btn_count = 0;
        {
            json_object_object_foreach(rom_obj, key, val) {
                (void)key; (void)val;
                btn_count++;
            }
        }
        
        if (btn_count > 0) {
            rom->controller_configs = malloc(sizeof(ControllerButtonConfig));
            if (!rom->controller_configs) {
                free(rom->rom_name);
                free(rom);
                return NULL;
            }
            
            rom->controller_configs[0].controller_name = strdup("*");  /* "*" means all controllers (legacy) */
            rom->controller_configs[0].buttons = malloc(sizeof(ButtonConfig) * btn_count);
            rom->controller_configs[0].button_count = 0;
            
            if (rom->controller_configs[0].buttons) {
                int i = 0;
                json_object_object_foreach(rom_obj, key, val) {
                    ButtonType button_type = button_name_to_enum(key);
                    if (button_type == BUTTON_MAX) {
                        fprintf(stderr, "Warning: Unknown button '%s'\n", key);
                        continue;
                    }
                    
                    rom->controller_configs[0].buttons[i].button = button_type;
                    if (parse_color(val, &rom->controller_configs[0].buttons[i].color) < 0) {
                        fprintf(stderr, "Warning: Invalid color for button '%s'\n", key);
                        continue;
                    }
                    i++;
                }
                rom->controller_configs[0].button_count = i;
            }
            rom->controller_config_count = 1;
        }
    }
    
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
    {
        json_object_object_foreach(roms_obj, key, val) {
            (void)key; (void)val;
            emulator->rom_count++;
        }
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
    {
        json_object_object_foreach(roms_obj, key, val) {
            RomConfig *rom = parse_rom(key, val);
            if (rom) {
                emulator->roms[i] = *rom;
                free(rom);
                i++;
            }
        }
    }
    emulator->rom_count = i;
    
    return emulator;
}

/* Parse i-pac controller from JSON */
static IpacController *parse_controller(struct json_object *ctrl_obj) {
    IpacController *controller = malloc(sizeof(IpacController));
    if (!controller) return NULL;
    
    /* Initialize runtime state fields */
    controller->usb_handle = NULL;
    controller->claimed_interface = -1;
    controller->driver_detached = 0;
    controller->default_buttons = NULL;
    controller->default_button_count = 0;
    controller->button_labels = NULL;
    
    struct json_object *device_obj, *vendor_obj, *product_obj, *pin_mappings_obj;
    
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
    
    /* Initialize pin mappings array */
    controller->pin_mappings = malloc(sizeof(PinMapping) * BUTTON_MAX);
    if (!controller->pin_mappings) {
        free(controller->device_name);
        free(controller);
        return NULL;
    }
    
    /* Set default pin mappings (in case not all buttons are configured) */
    for (int i = 0; i < BUTTON_MAX; i++) {
        controller->pin_mappings[i].r_pin = -1;
        controller->pin_mappings[i].g_pin = -1;
        controller->pin_mappings[i].b_pin = -1;
    }
    
    /* Parse pin mappings from JSON */
    if (json_object_object_get_ex(ctrl_obj, "pin_mappings", &pin_mappings_obj)) {
        json_object_object_foreach(pin_mappings_obj, button_name, pin_obj) {
            ButtonType button_type = button_name_to_enum(button_name);
            if (button_type == BUTTON_MAX) {
                fprintf(stderr, "Warning: Unknown button '%s' in pin_mappings\n", button_name);
                continue;
            }
            
            struct json_object *r_pin_obj, *g_pin_obj, *b_pin_obj;
            if (json_object_object_get_ex(pin_obj, "r_pin", &r_pin_obj) &&
                json_object_object_get_ex(pin_obj, "g_pin", &g_pin_obj) &&
                json_object_object_get_ex(pin_obj, "b_pin", &b_pin_obj)) {
                controller->pin_mappings[button_type].r_pin = json_object_get_int(r_pin_obj);
                controller->pin_mappings[button_type].g_pin = json_object_get_int(g_pin_obj);
                controller->pin_mappings[button_type].b_pin = json_object_get_int(b_pin_obj);
            } else {
                fprintf(stderr, "Warning: Invalid pin mapping for button '%s'\n", button_name);
            }
        }
    } else {
        fprintf(stderr, "Warning: No pin_mappings found in controller configuration\n");
    }
    
    /* Parse default button colors for this controller */
    struct json_object *default_obj;
    if (json_object_object_get_ex(ctrl_obj, "default", &default_obj)) {
        /* Count buttons */
        int btn_count = 0;
        json_object_object_foreach(default_obj, key, val) {
            (void)key; (void)val;
            btn_count++;
        }
        
        if (btn_count > 0) {
            controller->default_buttons = malloc(sizeof(ButtonConfig) * btn_count);
            if (controller->default_buttons) {
                int i = 0;
                json_object_object_foreach(default_obj, button_name, color_obj) {
                    ButtonType btn_type = button_name_to_enum(button_name);
                    if (btn_type == BUTTON_MAX) {
                        fprintf(stderr, "Warning: Unknown button '%s' in default colors\n", button_name);
                        continue;
                    }
                    
                    const char *color_str = json_object_get_string(color_obj);
                    RGBColor color = {0, 0, 0};
                    if (color_str && color_str[0] == '#' && strlen(color_str) == 7) {
                        unsigned int r, g, b;
                        sscanf(color_str + 1, "%02x%02x%02x", &r, &g, &b);
                        color = (RGBColor){r, g, b};
                    }
                    
                    controller->default_buttons[i].button = btn_type;
                    controller->default_buttons[i].color = color;
                    i++;
                }
                controller->default_button_count = i;
            }
        }
    }
    
    /* Parse button labels for this controller */
    struct json_object *labels_obj;
    if (json_object_object_get_ex(ctrl_obj, "button_labels", &labels_obj)) {
        controller->button_labels = calloc(BUTTON_MAX, sizeof(char *));
        if (controller->button_labels) {
            json_object_object_foreach(labels_obj, button_name, label_obj) {
                ButtonType btn_type = button_name_to_enum(button_name);
                if (btn_type == BUTTON_MAX) {
                    fprintf(stderr, "Warning: Unknown button '%s' in button_labels\n", button_name);
                    continue;
                }
                
                const char *label = json_object_get_string(label_obj);
                if (label) {
                    controller->button_labels[btn_type] = strdup(label);
                }
            }
        }
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
    config->animations_dir = NULL;
    config->idle_animation = NULL;
    
    /* Parse animations_dir */
    struct json_object *anim_dir_obj;
    if (json_object_object_get_ex(root, "animations_dir", &anim_dir_obj)) {
        config->animations_dir = strdup(json_object_get_string(anim_dir_obj));
    }
    
    /* Parse idle_animation */
    struct json_object *idle_anim_obj;
    if (json_object_object_get_ex(root, "idle_animation", &idle_anim_obj)) {
        config->idle_animation = strdup(json_object_get_string(idle_anim_obj));
    }
    
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
    
    /* Parse emulators */
    struct json_object *emulators_obj;
    if (json_object_object_get_ex(root, "emulators", &emulators_obj)) {
        /* Count emulators */
        {
            json_object_object_foreach(emulators_obj, key, val) {
                (void)key; (void)val;
                config->emulator_count++;
            }
        }
        
        /* Allocate emulator array */
        config->emulators = malloc(sizeof(EmulatorConfig) * config->emulator_count);
        
        /* Parse emulators */
        int i = 0;
        {
            json_object_object_foreach(emulators_obj, key, val) {
                EmulatorConfig *emulator = parse_emulator(key, val);
                if (emulator) {
                    config->emulators[i] = *emulator;
                    free(emulator);
                    i++;
                }
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
            free(config->controllers[i].pin_mappings);
            free(config->controllers[i].default_buttons);
            if (config->controllers[i].button_labels) {
                for (int j = 0; j < BUTTON_MAX; j++) {
                    free(config->controllers[i].button_labels[j]);
                }
                free(config->controllers[i].button_labels);
            }
        }
        free(config->controllers);
    }
    
    /* Free emulators */
    if (config->emulators) {
        for (int i = 0; i < config->emulator_count; i++) {
            EmulatorConfig *emulator = &config->emulators[i];
            free(emulator->emulator_name);
            
            if (emulator->roms) {
                for (int j = 0; j < emulator->rom_count; j++) {
                    free(emulator->roms[j].rom_name);
                    /* Free controller-specific button configs */
                    if (emulator->roms[j].controller_configs) {
                        for (int k = 0; k < emulator->roms[j].controller_config_count; k++) {
                            free(emulator->roms[j].controller_configs[k].controller_name);
                            free(emulator->roms[j].controller_configs[k].buttons);
                        }
                        free(emulator->roms[j].controller_configs);
                    }
                }
                free(emulator->roms);
            }
        }
        free(config->emulators);
    }
    
    /* Free animation settings */
    free(config->animations_dir);
    free(config->idle_animation);
    
    free(config);
}

/* Load a custom animation from a JSON file */
CustomAnimation *load_custom_animation(const char *filepath) {
    struct json_object *root;
    CustomAnimation *anim = NULL;
    
    root = json_object_from_file(filepath);
    if (!root) {
        fprintf(stderr, "Error: Could not load animation file '%s'\n", filepath);
        return NULL;
    }
    
    anim = calloc(1, sizeof(CustomAnimation));
    if (!anim) {
        json_object_put(root);
        return NULL;
    }
    
    /* Extract filename from filepath for reference */
    const char *filename = strrchr(filepath, '/');
    if (!filename) {
        filename = strrchr(filepath, '\\');
    }
    filename = filename ? filename + 1 : filepath;
    
    /* Remove .json extension for the animation name lookup */
    char *name_copy = strdup(filename);
    char *dot = strrchr(name_copy, '.');
    if (dot) *dot = '\0';
    anim->filename = strdup(name_copy);
    free(name_copy);
    
    /* Parse friendly name */
    struct json_object *name_obj;
    if (json_object_object_get_ex(root, "name", &name_obj)) {
        anim->name = strdup(json_object_get_string(name_obj));
    } else {
        anim->name = strdup(anim->filename);
    }
    
    /* Parse speed */
    struct json_object *speed_obj;
    if (json_object_object_get_ex(root, "speed", &speed_obj)) {
        anim->speed_ms = json_object_get_int(speed_obj);
    } else {
        anim->speed_ms = 50; /* Default 50ms */
    }
    
    /* Parse loop setting */
    struct json_object *loop_obj;
    if (json_object_object_get_ex(root, "loop", &loop_obj)) {
        anim->loop = json_object_get_boolean(loop_obj);
    } else {
        anim->loop = true; /* Default to looping */
    }
    
    /* Parse frames array */
    struct json_object *frames_obj;
    if (!json_object_object_get_ex(root, "frames", &frames_obj)) {
        fprintf(stderr, "Warning: No 'frames' array found in animation '%s'\n", filepath);
        json_object_put(root);
        free(anim->name);
        free(anim->filename);
        free(anim);
        return NULL;
    }
    
    int frame_count = json_object_array_length(frames_obj);
    if (frame_count <= 0) {
        fprintf(stderr, "Warning: Empty 'frames' array in animation '%s'\n", filepath);
        json_object_put(root);
        free(anim->name);
        free(anim->filename);
        free(anim);
        return NULL;
    }
    
    anim->frames = calloc(frame_count, sizeof(CustomAnimationFrame));
    if (!anim->frames) {
        json_object_put(root);
        free(anim->name);
        free(anim->filename);
        free(anim);
        return NULL;
    }
    
    anim->frame_count = 0;
    for (int i = 0; i < frame_count; i++) {
        struct json_object *frame_obj = json_object_array_get_idx(frames_obj, i);
        CustomAnimationFrame *frame = &anim->frames[anim->frame_count];
        
        /* Parse buttons array */
        struct json_object *buttons_obj;
        if (!json_object_object_get_ex(frame_obj, "buttons", &buttons_obj)) {
            fprintf(stderr, "Warning: Missing 'buttons' array in animation frame\n");
            continue;
        }
        
        int btn_count = json_object_array_length(buttons_obj);
        if (btn_count <= 0) {
            fprintf(stderr, "Warning: Empty 'buttons' array in animation frame\n");
            continue;
        }
        
        frame->buttons = calloc(btn_count, sizeof(ButtonColorPair));
        if (!frame->buttons) {
            continue;
        }
        
        frame->button_count = 0;
        for (int j = 0; j < btn_count; j++) {
            struct json_object *btn_obj = json_object_array_get_idx(buttons_obj, j);
            ButtonColorPair *pair = &frame->buttons[frame->button_count];
            
            /* Parse button name */
            struct json_object *button_name_obj;
            if (json_object_object_get_ex(btn_obj, "button", &button_name_obj)) {
                const char *button_str = json_object_get_string(button_name_obj);
                ButtonType btn = button_name_to_enum(button_str);
                if (btn == BUTTON_MAX) {
                    fprintf(stderr, "Warning: Unknown button '%s' in animation frame\n", button_str);
                    continue;
                }
                pair->button = btn;
            } else {
                fprintf(stderr, "Warning: Missing 'button' in buttons array\n");
                continue;
            }
            
            /* Parse color */
            struct json_object *color_obj;
            if (json_object_object_get_ex(btn_obj, "color", &color_obj)) {
                const char *color_str = json_object_get_string(color_obj);
                if (parse_color_string(color_str, &pair->color) < 0) {
                    fprintf(stderr, "Warning: Invalid color '%s' in animation frame\n", color_str);
                    pair->color = (RGBColor){0, 0, 0};
                }
            } else {
                pair->color = (RGBColor){0, 0, 0};
            }
            
            /* Parse controller index (-1 means all controllers) */
            struct json_object *controller_obj;
            if (json_object_object_get_ex(btn_obj, "controller", &controller_obj)) {
                pair->controller = json_object_get_int(controller_obj);
            } else {
                pair->controller = -1; /* Default: apply to all controllers */
            }
            
            frame->button_count++;
        }
        
        /* Parse fade */
        struct json_object *fade_obj;
        if (json_object_object_get_ex(frame_obj, "fade", &fade_obj)) {
            frame->fade = json_object_get_boolean(fade_obj);
        } else {
            frame->fade = false;
        }
        
        /* Parse fade_speed_ms */
        struct json_object *fade_speed_obj;
        if (json_object_object_get_ex(frame_obj, "fade_speed_ms", &fade_speed_obj)) {
            frame->fade_speed_ms = json_object_get_int(fade_speed_obj);
        } else {
            frame->fade_speed_ms = 0;
        }
        
        anim->frame_count++;
    }
    
    json_object_put(root);
    
    printf("Loaded custom animation '%s' (%s) with %d frames\n", 
           anim->name, anim->filename, anim->frame_count);
    
    return anim;
}

/* Free a custom animation */
void free_custom_animation(CustomAnimation *anim) {
    if (!anim) return;
    
    if (anim->frames) {
        for (int i = 0; i < anim->frame_count; i++) {
            free(anim->frames[i].buttons);
        }
        free(anim->frames);
    }
    free(anim->name);
    free(anim->filename);
    free(anim);
}

/* Load all custom animations from a directory */
CustomAnimationRegistry *load_custom_animation_registry(const char *animations_dir) {
    CustomAnimationRegistry *registry = calloc(1, sizeof(CustomAnimationRegistry));
    if (!registry) return NULL;
    
    registry->animations_dir = strdup(animations_dir);
    registry->animations = NULL;
    registry->animation_count = 0;
    
    /* Open directory and count .json files */
    DIR *dir = opendir(animations_dir);
    if (!dir) {
        fprintf(stderr, "Warning: Could not open animations directory '%s'\n", animations_dir);
        return registry;
    }
    
    /* First pass: count JSON files */
    struct dirent *entry;
    int json_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".json") == 0) {
            json_count++;
        }
    }
    
    if (json_count == 0) {
        closedir(dir);
        return registry;
    }
    
    /* Allocate array for animations */
    registry->animations = calloc(json_count, sizeof(CustomAnimation));
    if (!registry->animations) {
        closedir(dir);
        return registry;
    }
    
    /* Second pass: load each animation */
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".json") == 0) {
            /* Construct full path */
            size_t path_len = strlen(animations_dir) + strlen(entry->d_name) + 2;
            char *filepath = malloc(path_len);
            if (!filepath) continue;
            
            snprintf(filepath, path_len, "%s/%s", animations_dir, entry->d_name);
            
            CustomAnimation *anim = load_custom_animation(filepath);
            if (anim) {
                registry->animations[registry->animation_count] = *anim;
                free(anim); /* Structure was copied, free the container */
                registry->animation_count++;
            }
            
            free(filepath);
        }
    }
    
    closedir(dir);
    
    printf("Loaded %d custom animations from '%s'\n", 
           registry->animation_count, animations_dir);
    
    return registry;
}

/* Free the custom animation registry */
void free_custom_animation_registry(CustomAnimationRegistry *registry) {
    if (!registry) return;
    
    if (registry->animations) {
        for (int i = 0; i < registry->animation_count; i++) {
            /* Free buttons arrays in each frame */
            if (registry->animations[i].frames) {
                for (int j = 0; j < registry->animations[i].frame_count; j++) {
                    free(registry->animations[i].frames[j].buttons);
                }
                free(registry->animations[i].frames);
            }
            free(registry->animations[i].name);
            free(registry->animations[i].filename);
        }
        free(registry->animations);
    }
    
    free(registry->animations_dir);
    free(registry);
}

/* Find a custom animation by name (filename without extension) */
CustomAnimation *find_custom_animation(CustomAnimationRegistry *registry, const char *name) {
    if (!registry || !name) return NULL;
    
    for (int i = 0; i < registry->animation_count; i++) {
        /* Match by filename (without extension) */
        if (strcasecmp(registry->animations[i].filename, name) == 0) {
            return &registry->animations[i];
        }
        /* Also match by friendly name */
        if (strcasecmp(registry->animations[i].name, name) == 0) {
            return &registry->animations[i];
        }
    }
    
    return NULL;
}
