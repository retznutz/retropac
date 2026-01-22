#ifndef RETROPAC_H
#define RETROPAC_H

#include <stdint.h>

/* Constants */
#define DEFAULT_CONFIG_NAME "__menu_default__"

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
} Config;

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

#endif /* RETROPAC_H */
