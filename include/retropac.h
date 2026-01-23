#ifndef RETROPAC_H
#define RETROPAC_H

#include <stdint.h>
#include <stdbool.h>

/* Constants */
#define DEFAULT_CONFIG_NAME "__menu_default__"
#define MAX_ANIMATION_BUTTONS 64

/* Button enumeration for arcade controls */
typedef enum {
    /* Coin buttons */
    P1_COIN = 0,
    P2_COIN,
    P3_COIN,
    P4_COIN,
    
    /* Start buttons */
    P1_START,
    P2_START,
    P3_START,
    P4_START,
    
    /* Player 1 buttons */
    P1_BUTTON1,
    P1_BUTTON2,
    P1_BUTTON3,
    P1_BUTTON4,
    P1_BUTTON5,
    P1_BUTTON6,
    
    /* Player 2 buttons */
    P2_BUTTON1,
    P2_BUTTON2,
    P2_BUTTON3,
    P2_BUTTON4,
    P2_BUTTON5,
    P2_BUTTON6,
    
    /* Player 3 buttons */
    P3_BUTTON1,
    P3_BUTTON2,
    P3_BUTTON3,
    P3_BUTTON4,
    P3_BUTTON5,
    P3_BUTTON6,
    
    /* Player 4 buttons */
    P4_BUTTON1,
    P4_BUTTON2,
    P4_BUTTON3,
    P4_BUTTON4,
    P4_BUTTON5,
    P4_BUTTON6,
    
    /* Joysticks */
    P1_JOYSTICK,
    P2_JOYSTICK,
    P3_JOYSTICK,
    P4_JOYSTICK,
    
    /* Trackballs */
    P1_TRACKBALL,
    P2_TRACKBALL,
    P3_TRACKBALL,
    P4_TRACKBALL,
    
    BUTTON_MAX
} ButtonType;

/* RGB color structure */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBColor;

/* Pin mapping for RGB LEDs */
typedef struct {
    int r_pin;  /* Pin for red channel */
    int g_pin;  /* Pin for green channel */
    int b_pin;  /* Pin for blue channel */
} PinMapping;

/* Button configuration */
typedef struct {
    ButtonType button;
    RGBColor color;
} ButtonConfig;

/* ROM configuration */
typedef struct {
    char *rom_name;
    ButtonConfig *buttons;
    int button_count;
} RomConfig;

/* Emulator configuration */
typedef struct {
    char *emulator_name;
    RomConfig *roms;
    int rom_count;
} EmulatorConfig;

/* i-pac controller information */
typedef struct {
    char *device_name;
    uint16_t vendor_id;
    uint16_t product_id;
    PinMapping *pin_mappings;  /* Array of pin mappings for each button type */
} IpacController;

/* Main configuration structure */
typedef struct {
    IpacController *controllers;
    int controller_count;
    EmulatorConfig *emulators;
    int emulator_count;
    RomConfig *default_config;  /* Top-level default button configuration */
    char *animations_dir;       /* Directory containing custom animation files */
    char *idle_animation;       /* Animation name to play during idle/attract mode */
} Config;

/* Animation types */
typedef enum {
    ANIM_NONE = 0,
    ANIM_RAINBOW,       /* Rotate rainbow colors across buttons */
    ANIM_BREATHING,     /* Fade in/out (pulse) effect */
    ANIM_CHASE,         /* Running light effect */
    ANIM_SPARKLE,       /* Random sparkle effect */
    ANIM_COLOR_CYCLE,   /* Cycle through colors on all buttons */
    ANIM_STATIC,        /* Static colors (no animation) */
    ANIM_CUSTOM         /* Custom animation loaded from JSON file */
} AnimationType;

/* Animation configuration */
typedef struct {
    AnimationType type;
    int speed_ms;           /* Delay between frames in milliseconds */
    RGBColor base_color;    /* Base color for breathing/chase effects */
    RGBColor *colors;       /* Array of colors for color_cycle */
    int color_count;
    ButtonType *buttons;    /* Which buttons to animate (NULL = all) */
    int button_count;
} AnimationConfig;

/* Button-color pair for custom animation frames */
typedef struct {
    ButtonType button;      /* Which button to set */
    RGBColor color;         /* Target color for this button */
} ButtonColorPair;

/* Custom animation frame (from JSON file) */
typedef struct {
    ButtonColorPair *buttons;   /* Array of button-color pairs */
    int button_count;           /* Number of buttons in this frame */
    bool fade;                  /* Whether to fade to the colors */
    int fade_speed_ms;          /* Fade duration in milliseconds */
    int delay_ms;               /* Delay before this frame executes (0 = immediate) */
} CustomAnimationFrame;

/* Custom animation (loaded from separate JSON file) */
typedef struct {
    char *name;                     /* Friendly name of the animation */
    char *filename;                 /* Source filename (without path) */
    int speed_ms;                   /* Overall speed/timing between frames */
    bool loop;                      /* Whether animation loops */
    CustomAnimationFrame *frames;   /* Array of animation frames */
    int frame_count;                /* Number of frames in the animation */
} CustomAnimation;

/* Custom animation registry */
typedef struct {
    CustomAnimation *animations;    /* Array of loaded custom animations */
    int animation_count;            /* Number of loaded animations */
    char *animations_dir;           /* Directory where animation files are stored */
} CustomAnimationRegistry;

/* Animation state (runtime) */
typedef struct {
    bool running;
    AnimationConfig *config;
    CustomAnimation *custom_anim;   /* For custom animations */
    int frame;
    int custom_frame_idx;           /* Current frame index for custom animations */
    int ipac_handle;
    PinMapping *pin_mappings;
    ButtonConfig *button_states;  /* Current state of all buttons */
    int total_buttons;
} AnimationState;

/* Function prototypes */

/* Config parsing */
Config *load_config(const char *filename);
void free_config(Config *config);

/* Button name to enum conversion */
ButtonType button_name_to_enum(const char *name);
const char *button_enum_to_name(ButtonType button);

/* i-pac controller communication */
int ipac_init(IpacController *controller);
int ipac_set_led(int handle, ButtonType button, RGBColor color, PinMapping *pin_mappings);
int ipac_clear_all_leds(int handle, PinMapping *pin_mappings);
int ipac_set_all_leds(int handle, ButtonConfig *buttons, int count, PinMapping *pin_mappings);
void ipac_close(int handle);

/* ROM name extraction from path */
char *extract_rom_name(const char *rom_path);

/* Animation functions */
AnimationState *animation_create(AnimationConfig *config, int ipac_handle, 
                                  PinMapping *pin_mappings, 
                                  ButtonConfig *initial_buttons, int button_count);
void animation_destroy(AnimationState *state);
void animation_run(AnimationState *state);  /* Blocking - runs until stopped */
void animation_stop(AnimationState *state);
void animation_step(AnimationState *state); /* Single frame update */

/* Animation config management */
void free_animation_config(AnimationConfig *config);
AnimationType animation_type_from_string(const char *name);
const char *animation_type_to_string(AnimationType type);

/* Custom animation functions */
CustomAnimation *load_custom_animation(const char *filepath);
CustomAnimationRegistry *load_custom_animation_registry(const char *animations_dir);
void free_custom_animation(CustomAnimation *anim);
void free_custom_animation_registry(CustomAnimationRegistry *registry);
CustomAnimation *find_custom_animation(CustomAnimationRegistry *registry, const char *name);
AnimationState *animation_create_custom(CustomAnimation *custom_anim, int ipac_handle,
                                         PinMapping *pin_mappings,
                                         ButtonConfig *initial_buttons, int button_count);
void animation_step_custom(AnimationState *state);

/* Signal handling for graceful shutdown */
void setup_signal_handlers(void);
bool should_exit(void);

#endif /* RETROPAC_H */
