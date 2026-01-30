#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "retropac.h"

#define MODE_DEFAULT "default"
#define MODE_ANIMATE "animate"
#define PID_FILE "/tmp/retropac.pid"

/* Global quiet mode flag - suppresses console output */
int quiet_mode = 0;

/* Kill any existing retropac daemon */
static void kill_existing_daemon(void) {
    FILE *fp = fopen(PID_FILE, "r");
    if (!fp) {
        return;  /* No PID file, no daemon running */
    }
    
    pid_t old_pid;
    if (fscanf(fp, "%d", &old_pid) == 1) {
        fclose(fp);
        
        /* Check if process exists and is retropac */
        if (kill(old_pid, 0) == 0) {
            if (!quiet_mode) printf("Killing existing retropac daemon (PID %d)...\n", old_pid);
            kill(old_pid, SIGTERM);
            
            /* Wait for it to exit gracefully (up to 500ms) */
            /* This gives the daemon time to properly close USB and reattach kernel drivers */
            for (int i = 0; i < 5; i++) {
                usleep(100000);  /* 100ms */
                if (kill(old_pid, 0) != 0) {
                    if (!quiet_mode) printf("Daemon terminated gracefully\n");
                    break;
                }
            }
            
            /* Force kill if still running (but this skips cleanup!) */
            if (kill(old_pid, 0) == 0) {
                if (!quiet_mode) fprintf(stderr, "Warning: Daemon did not exit gracefully, forcing kill\n");
                kill(old_pid, SIGKILL);
                usleep(100000);  /* 100ms */
            }
        }
    } else {
        fclose(fp);
    }
    
    /* Remove stale PID file */
    unlink(PID_FILE);
}

/* Write current PID to file */
static void write_pid_file(void) {
    FILE *fp = fopen(PID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

/* Remove PID file on exit */
static void remove_pid_file(void) {
    unlink(PID_FILE);
}

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

/* Build a default RomConfig from controller defaults (each controller gets its own config) */
static RomConfig *build_default_config(Config *config) {
    if (!config || config->controller_count == 0) return NULL;
    
    /* Count controllers with default configs */
    int controllers_with_defaults = 0;
    for (int c = 0; c < config->controller_count; c++) {
        if (config->controllers[c].default_button_count > 0) {
            controllers_with_defaults++;
        }
    }
    
    if (controllers_with_defaults == 0) return NULL;
    
    RomConfig *rom_config = malloc(sizeof(RomConfig));
    if (!rom_config) return NULL;
    
    rom_config->rom_name = strdup(DEFAULT_CONFIG_NAME);
    rom_config->controller_configs = malloc(sizeof(ControllerButtonConfig) * controllers_with_defaults);
    rom_config->controller_config_count = 0;
    
    if (!rom_config->controller_configs) {
        free(rom_config->rom_name);
        free(rom_config);
        return NULL;
    }
    
    /* Create per-controller configs from their defaults */
    for (int c = 0; c < config->controller_count; c++) {
        IpacController *ctrl = &config->controllers[c];
        if (ctrl->default_button_count > 0) {
            ControllerButtonConfig *cfg = &rom_config->controller_configs[rom_config->controller_config_count];
            cfg->controller_name = strdup(ctrl->device_name);
            cfg->buttons = malloc(sizeof(ButtonConfig) * ctrl->default_button_count);
            cfg->button_count = ctrl->default_button_count;
            
            if (cfg->buttons) {
                memcpy(cfg->buttons, ctrl->default_buttons, 
                       sizeof(ButtonConfig) * ctrl->default_button_count);
                rom_config->controller_config_count++;
            }
        }
    }
    
    return rom_config;
}

/* Build a flat list of all buttons from a ROM config (for animations that broadcast) */
static ButtonConfig *build_flat_button_list(RomConfig *rom_config, int *out_count) {
    if (!rom_config || rom_config->controller_config_count == 0) {
        *out_count = 0;
        return NULL;
    }
    
    /* Count total buttons */
    int total = 0;
    for (int c = 0; c < rom_config->controller_config_count; c++) {
        total += rom_config->controller_configs[c].button_count;
    }
    
    if (total == 0) {
        *out_count = 0;
        return NULL;
    }
    
    ButtonConfig *buttons = malloc(sizeof(ButtonConfig) * total);
    if (!buttons) {
        *out_count = 0;
        return NULL;
    }
    
    /* Copy all buttons */
    int idx = 0;
    for (int c = 0; c < rom_config->controller_config_count; c++) {
        ControllerButtonConfig *cfg = &rom_config->controller_configs[c];
        for (int b = 0; b < cfg->button_count; b++) {
            buttons[idx++] = cfg->buttons[b];
        }
    }
    
    *out_count = total;
    return buttons;
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
        printf("Emulator '%s' not found in config, using controller defaults\n", emulator_name);
        return NULL;  /* Caller will use controller defaults */
    }
    
    /* Find specific ROM config */
    RomConfig *rom = NULL;
    for (int i = 0; i < emulator->rom_count; i++) {
        if (strcmp(emulator->roms[i].rom_name, rom_name) == 0) {
            rom = &emulator->roms[i];
            printf("Using ROM-specific configuration for '%s'\n", rom_name);
            return rom;
        }
    }
    
    /* ROM not found - try emulator's default config */
    for (int i = 0; i < emulator->rom_count; i++) {
        if (strcmp(emulator->roms[i].rom_name, "default") == 0) {
            rom = &emulator->roms[i];
            printf("ROM '%s' not found, using emulator '%s' default configuration\n", 
                   rom_name, emulator_name);
            return rom;
        }
    }
    
    /* No emulator default - fall back to controller defaults */
    printf("No default for emulator '%s', using controller defaults\n", emulator_name);
    return NULL;
}

/* Print usage information */
static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [options] <emulator> <rom_path> [mode]\n\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --config <path>    Config file path (default: %s)\n", DEFAULT_CONFIG_PATH);
    fprintf(stderr, "  --animate <type>   Run animation (rainbow, breathing, chase, sparkle, color_cycle)\n");
    fprintf(stderr, "  --custom <name>    Run custom animation by filename (without .json extension)\n");
    fprintf(stderr, "  --speed <ms>       Animation speed in milliseconds (default: 50)\n");
    fprintf(stderr, "  --color <hex>      Base color for animations (e.g., #FF0000)\n");
    fprintf(stderr, "  --set-button <name> <color>  Set a single button LED (e.g., --set-button P1_BUTTON1 #FF0000)\n");
    fprintf(stderr, "  --daemon           Run as daemon (for animations in background)\n");
    fprintf(stderr, "  --quiet            Suppress all console output\n");
    fprintf(stderr, "  --help             Show this help message\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "  Run with specific game:\n");
    fprintf(stderr, "    %s mame /path/to/sf2.zip\n", prog_name);
    fprintf(stderr, "  Run with default config (EmulationStation menu):\n");
    fprintf(stderr, "    %s default default default\n", prog_name);
    fprintf(stderr, "  Run rainbow animation:\n");
    fprintf(stderr, "    %s --animate rainbow default default default\n", prog_name);
    fprintf(stderr, "  Run breathing animation with red color:\n");
    fprintf(stderr, "    %s --animate breathing --color '#FF0000' --speed 30 default default default\n", prog_name);
    fprintf(stderr, "  Run custom animation by name:\n");
    fprintf(stderr, "    %s --custom rainbow_wave default default default\n", prog_name);
    fprintf(stderr, "  Run idle animation from config (daemon mode):\n");
    fprintf(stderr, "    %s --custom idle --daemon default default default\n", prog_name);
    fprintf(stderr, "  Set a single button LED:\n");
    fprintf(stderr, "    %s --set-button P1_BUTTON1 '#FFFFFF'\n", prog_name);
}

int main(int argc, char *argv[]) {
    const char *config_file = DEFAULT_CONFIG_PATH;
    const char *emulator_name = NULL;
    const char *rom_path = NULL;
    const char *mode = NULL;
    char *rom_name = NULL;
    Config *config = NULL;
    RomConfig *rom_config = NULL;
    int rom_config_allocated = 0;  /* Track if rom_config was dynamically allocated */
    AnimationConfig *anim_config = NULL;
    AnimationState *anim_state = NULL;
    CustomAnimationRegistry *anim_registry = NULL;
    CustomAnimation *custom_anim = NULL;
    int controllers_initialized = 0;
    int exit_code = 0;
    
    /* Animation options */
    AnimationType anim_type = ANIM_NONE;
    int anim_speed = 50;
    RGBColor anim_color = {255, 255, 255};
    int run_as_daemon = 0;
    const char *custom_anim_name = NULL;
    
    /* Single button test mode */
    const char *set_button_name = NULL;
    RGBColor set_button_color = {255, 255, 255};
    
    /* Parse command line options */
    static struct option long_options[] = {
        {"config",     required_argument, 0, 'f'},
        {"animate",    required_argument, 0, 'a'},
        {"custom",     required_argument, 0, 'C'},
        {"speed",      required_argument, 0, 's'},
        {"color",      required_argument, 0, 'c'},
        {"set-button", required_argument, 0, 'b'},
        {"daemon",     no_argument,       0, 'd'},
        {"quiet",      no_argument,       0, 'q'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    while ((opt = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                config_file = optarg;
                break;
            case 'a':
                anim_type = animation_type_from_string(optarg);
                if (anim_type == ANIM_NONE) {
                    fprintf(stderr, "Warning: Unknown animation type '%s', using rainbow\n", optarg);
                    anim_type = ANIM_RAINBOW;
                }
                break;
            case 'C':
                custom_anim_name = optarg;
                anim_type = ANIM_CUSTOM;
                break;
            case 's':
                anim_speed = atoi(optarg);
                if (anim_speed <= 0) anim_speed = 50;
                break;
            case 'c':
                if (optarg[0] == '#' && strlen(optarg) == 7) {
                    unsigned int r, g, b;
                    sscanf(optarg + 1, "%02x%02x%02x", &r, &g, &b);
                    anim_color = (RGBColor){r, g, b};
                }
                break;
            case 'b':
                /* --set-button expects button name, then color as next argument */
                set_button_name = optarg;
                /* Look for color in next argument */
                if (optind < argc && argv[optind][0] == '#') {
                    const char *color_arg = argv[optind];
                    if (strlen(color_arg) == 7) {
                        unsigned int r, g, b;
                        sscanf(color_arg + 1, "%02x%02x%02x", &r, &g, &b);
                        set_button_color = (RGBColor){r, g, b};
                    }
                    optind++;
                }
                break;
            case 'd':
                run_as_daemon = 1;
                break;
            case 'q':
                quiet_mode = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Redirect stdout and stderr to /dev/null if quiet mode */
    if (quiet_mode) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
    }
    
    /* Always kill any existing daemon first - this makes retropac self-managing */
    kill_existing_daemon();
    
    printf("RetroPac - Ultimarc i-pac LED Controller v1.1\n");
    printf("==============================================\n\n");
    
    /* Handle --set-button mode (doesn't require emulator/rom args) */
    if (set_button_name) {
        printf("Set button mode: %s -> RGB(%d, %d, %d)\n", 
               set_button_name, set_button_color.r, set_button_color.g, set_button_color.b);
        
        /* Load configuration */
        printf("Loading configuration from %s...\n", config_file);
        config = load_config(config_file);
        if (!config) {
            fprintf(stderr, "Error: Could not load configuration\n");
            return 1;
        }
        
        /* Initialize i-pac controller(s) */
        if (config->controller_count > 0) {
            controllers_initialized = ipac_init_all(config->controllers, config->controller_count);
            if (controllers_initialized == 0) {
                fprintf(stderr, "Error: Could not initialize any i-pac controllers\n");
                free_config(config);
                return 1;
            }
            
            /* Set the single button LED on all controllers */
            ButtonType btn_type = button_name_to_enum(set_button_name);
            if (btn_type == BUTTON_MAX) {
                fprintf(stderr, "Error: Unknown button name '%s'\n", set_button_name);
                ipac_close_all(config->controllers, config->controller_count);
                free_config(config);
                return 1;
            }
            
            ButtonConfig btn_config = { btn_type, set_button_color };
            if (ipac_set_all_leds_all(config->controllers, config->controller_count, &btn_config, 1) < 0) {
                fprintf(stderr, "Warning: Could not set LED\n");
            } else {
                printf("LED set successfully on %d controller(s)\n", controllers_initialized);
            }
            
            ipac_close_all(config->controllers, config->controller_count);
        } else {
            fprintf(stderr, "Error: No i-pac controllers defined in configuration\n");
        }
        
        free_config(config);
        return 0;
    }
    
    /* Check remaining arguments */
    if (argc - optind < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    emulator_name = argv[optind];
    rom_path = argv[optind + 1];
    if (argc - optind >= 3) {
        mode = argv[optind + 2];
    }
    
    printf("Emulator: %s\n", emulator_name);
    printf("ROM path: %s\n", rom_path);
    if (anim_type != ANIM_NONE) {
        printf("Animation: %s (speed: %dms)\n", animation_type_to_string(anim_type), anim_speed);
    };
    
    /* Check if we're in default mode first */
    if (mode && strcmp(mode, MODE_DEFAULT) == 0) {
        printf("Mode: %s\n\n", mode);
    } else if (mode == NULL && anim_type != ANIM_NONE) {
        /* Animation without mode - treat as default */
        mode = MODE_DEFAULT;
        printf("Mode: %s (animation)\n\n", mode);
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
    
    /* Daemonize early if requested - before opening USB handles */
    if (run_as_daemon) {
        printf("Running as daemon...\n");
        /* Use daemon(1, 0) to preserve working directory for relative paths */
        if (daemon(1, 0) != 0) {
            perror("Failed to daemonize");
            exit_code = 1;
            goto cleanup;
        }
        /* Write PID file immediately after daemonizing */
        write_pid_file();
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
    
    /* Count controllers with default configs */
    int controllers_with_defaults = 0;
    for (int i = 0; i < config->controller_count; i++) {
        if (config->controllers[i].default_button_count > 0) {
            controllers_with_defaults++;
        }
    }
    printf("  Controllers with defaults: %d\n\n", controllers_with_defaults);
    
    /* Find ROM configuration */
    /* If mode is "default", build configuration from controller defaults */
    if (mode && strcmp(mode, MODE_DEFAULT) == 0) {
        rom_config = build_default_config(config);
        if (rom_config) {
            rom_config_allocated = 1;
            printf("Using controller default configuration (%d controller configs)\n\n", 
                   rom_config->controller_config_count);
        } else {
            fprintf(stderr, "Error: No default configuration found in any controller\n");
            exit_code = 1;
            goto cleanup;
        }
    } else {
        rom_config = find_rom_config(config, emulator_name, rom_name);
        if (!rom_config) {
            /* Fall back to controller defaults */
            rom_config = build_default_config(config);
            if (!rom_config) {
                fprintf(stderr, "Error: No configuration available (no ROM, emulator, or controller defaults)\n");
                exit_code = 1;
                goto cleanup;
            }
            rom_config_allocated = 1;
            printf("Using controller default configuration (%d controller configs)\n\n", 
                   rom_config->controller_config_count);
        }
    }
    printf("\nFound ROM configuration with %d controller config(s)\n\n", rom_config->controller_config_count);
    
    /* Initialize i-pac controller(s) */
    if (config->controller_count > 0) {
        printf("Initializing %d i-pac controller(s)...\n", config->controller_count);
        controllers_initialized = ipac_init_all(config->controllers, config->controller_count);
        if (controllers_initialized == 0) {
            fprintf(stderr, "Warning: Could not initialize any i-pac controllers\n");
            fprintf(stderr, "Continuing in simulation mode (no hardware control)\n\n");
        } else {
            printf("Successfully initialized %d of %d controller(s)\n\n", 
                   controllers_initialized, config->controller_count);
        }
    } else {
        fprintf(stderr, "Warning: No i-pac controllers defined in configuration\n");
        fprintf(stderr, "Running in simulation mode\n\n");
    }
    
    /* Set LEDs (skip if animation will run - avoids brief flash of default colors) */
    if (anim_type == ANIM_NONE) {
        if (controllers_initialized > 0) {
            if (ipac_apply_rom_config(config->controllers, config->controller_count, 
                                       rom_config->controller_configs, 
                                       rom_config->controller_config_count) < 0) {
                fprintf(stderr, "Warning: Some LEDs could not be set\n");
            }
        } else {
            /* Simulation mode - just print what would be set */
            printf("Simulation mode - would set the following LEDs:\n");
            for (int c = 0; c < rom_config->controller_config_count; c++) {
                ControllerButtonConfig *cfg = &rom_config->controller_configs[c];
                printf("  Controller: %s\n", cfg->controller_name);
                for (int i = 0; i < cfg->button_count; i++) {
                    printf("    %s -> RGB(%d, %d, %d)\n",
                           button_enum_to_name(cfg->buttons[i].button),
                           cfg->buttons[i].color.r,
                           cfg->buttons[i].color.g,
                           cfg->buttons[i].color.b);
                }
            }
        }
    }

    /* Run animation if requested */
    if (anim_type != ANIM_NONE) {
        /* Setup signal handlers for graceful shutdown */
        setup_signal_handlers();
        
        /* Build flat button list for animations (they broadcast to all controllers) */
        int flat_button_count = 0;
        ButtonConfig *flat_buttons = build_flat_button_list(rom_config, &flat_button_count);
        
        /* Handle custom animations */
        if (anim_type == ANIM_CUSTOM) {
            /* Load animation registry from animations directory */
            const char *anim_dir = config->animations_dir ? config->animations_dir : "animations";
            anim_registry = load_custom_animation_registry(anim_dir);
            if (!anim_registry) {
                fprintf(stderr, "Error: Could not load animation registry from '%s'\n", anim_dir);
                free(flat_buttons);
                exit_code = 1;
                goto cleanup;
            }
            
            /* If custom_anim_name is "idle", use the idle_animation from config */
            const char *anim_to_find = custom_anim_name;
            if (strcmp(custom_anim_name, "idle") == 0 && config->idle_animation) {
                anim_to_find = config->idle_animation;
                printf("Using idle animation: %s\n", anim_to_find);
            }
            
            /* Find the requested animation */
            custom_anim = find_custom_animation(anim_registry, anim_to_find);
            if (!custom_anim) {
                fprintf(stderr, "Error: Custom animation '%s' not found\n", anim_to_find);
                free(flat_buttons);
                exit_code = 1;
                goto cleanup;
            }
            
            printf("Running custom animation: %s (%s)\n", custom_anim->name, custom_anim->filename);
            
            /* Create animation state for custom animation */
            anim_state = animation_create_custom(custom_anim, 
                                                  config->controllers, config->controller_count,
                                                  flat_buttons, flat_button_count);
        } else {
            /* Create built-in animation config */
            anim_config = calloc(1, sizeof(AnimationConfig));
            if (!anim_config) {
                fprintf(stderr, "Error: Could not allocate animation config\n");
                free(flat_buttons);
                exit_code = 1;
                goto cleanup;
            }
            anim_config->type = anim_type;
            anim_config->speed_ms = anim_speed;
            anim_config->base_color = anim_color;
            
            /* Create animation state for built-in animation */
            anim_state = animation_create(anim_config, 
                                           config->controllers, config->controller_count,
                                           flat_buttons, flat_button_count);
        }
        
        free(flat_buttons);  /* Animation state copies the data */
        
        if (!anim_state) {
            fprintf(stderr, "Error: Could not create animation state\n");
            exit_code = 1;
            goto cleanup;
        }
        
        /* Run animation loop (blocks until signal) */
        animation_run(anim_state);
        
        /* Clear LEDs on exit */
        if (controllers_initialized > 0) {
            ipac_clear_all_leds_all(config->controllers, config->controller_count);
        }
    }
    
    printf("\nRetroPac completed successfully\n");
    
cleanup:
    /* Clean up PID file if we're running as daemon */
    if (run_as_daemon) {
        remove_pid_file();
    }
    if (anim_state) {
        animation_destroy(anim_state);
    }
    if (anim_config) {
        free_animation_config(anim_config);
    }
    if (anim_registry) {
        free_custom_animation_registry(anim_registry);
    }
    if (controllers_initialized > 0) {
        ipac_close_all(config->controllers, config->controller_count);
    }
    if (rom_config_allocated && rom_config) {
        free(rom_config->rom_name);
        /* Free per-controller button configs */
        if (rom_config->controller_configs) {
            for (int i = 0; i < rom_config->controller_config_count; i++) {
                free(rom_config->controller_configs[i].controller_name);
                free(rom_config->controller_configs[i].buttons);
            }
            free(rom_config->controller_configs);
        }
        free(rom_config);
    }
    if (rom_name) {
        free(rom_name);
    }
    if (config) {
        free_config(config);
    }
    
    return exit_code;
}
