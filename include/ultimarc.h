/**
 * @file ultimarc.h
 * @brief Ultimarc device definitions and types
 * 
 * This header defines constants and types for all supported Ultimarc devices,
 * based on the official PacDrive SDK specifications.
 * 
 * Supported devices:
 * - PacDrive (16 LED channels, on/off only)
 * - U-HID (16 LED channels, on/off only)
 * - NanoLED (60 LED channels, 256 brightness levels, 20 RGB LEDs)
 * - PacLED64 (64 LED channels, 256 brightness levels, 21 RGB LEDs)
 * - I-PAC Ultimate I/O (96 LED channels, 256 brightness levels, 32 RGB LEDs)
 * - ServoStik (4-way/8-way switching)
 * - USB Button (RGB button with state reading)
 */

#ifndef ULTIMARC_H
#define ULTIMARC_H

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * Vendor and Product IDs
 * ======================================================================== */

#define ULTIMARC_VID                0xD209  /* Ultimarc Vendor ID */

/* PacDrive uses its version number to identify different devices */
#define PID_PACDRIVE                0x1500

/* U-HID range (up to 8 devices) */
#define PID_UHID_LO                 0x1501
#define PID_UHID_HI                 0x1508

/* NanoLED range (up to 4 devices) */
#define PID_NANOLED_LO              0x1481
#define PID_NANOLED_HI              0x1484

/* PacLED64 range (up to 8 devices) */
#define PID_PACLED64_LO             0x1401
#define PID_PACLED64_HI             0x1408

/* I-PAC Ultimate I/O range (up to 4 devices) */
#define PID_IPAC_ULTIMATE_IO_LO     0x0410
#define PID_IPAC_ULTIMATE_IO_HI     0x0413

/* Single-device products */
#define PID_SERVOSTIK               0x1700
#define PID_USBBUTTON               0x1200

/* ========================================================================
 * Device Type Detection Macros
 * ======================================================================== */

#define IS_ULTIMARC_VID(vid)        ((vid) == ULTIMARC_VID)

#define IS_PACDRIVE(pid)            ((pid) == PID_PACDRIVE)
#define IS_UHID(pid)                ((pid) >= PID_UHID_LO && (pid) <= PID_UHID_HI)
#define IS_NANOLED(pid)             ((pid) >= PID_NANOLED_LO && (pid) <= PID_NANOLED_HI)
#define IS_PACLED64(pid)            ((pid) >= PID_PACLED64_LO && (pid) <= PID_PACLED64_HI)
#define IS_IPAC_ULTIMATE_IO(pid)    ((pid) >= PID_IPAC_ULTIMATE_IO_LO && (pid) <= PID_IPAC_ULTIMATE_IO_HI)
#define IS_SERVOSTIK(pid)           ((pid) == PID_SERVOSTIK)
#define IS_USBBUTTON(pid)           ((pid) == PID_USBBUTTON)

/* Device capability groups */
#define IS_16LED_DEVICE(type)       ((type) == DEVICE_PACDRIVE || (type) == DEVICE_UHID)
#define IS_64LED_DEVICE(type)       ((type) == DEVICE_NANOLED || (type) == DEVICE_PACLED64 || (type) == DEVICE_IPAC_ULTIMATE_IO)

/* ========================================================================
 * LED Channel Counts
 * ======================================================================== */

#define LED_COUNT_PACDRIVE          16
#define LED_COUNT_UHID              16
#define LED_COUNT_NANOLED           60
#define LED_COUNT_PACLED64          64
#define LED_COUNT_IPAC_ULTIMATE_IO  96

/* RGB LED counts (for devices with PWM intensity control) */
#define RGB_COUNT_NANOLED           20
#define RGB_COUNT_PACLED64          21
#define RGB_COUNT_IPAC_ULTIMATE_IO  32

/* ========================================================================
 * USB Interface Constants
 * ======================================================================== */

/* I-PAC Ultimate I/O uses interface 2 for LED control */
#define IPAC_ULTIMATE_LED_INTERFACE 2

/* PacLED64 and NanoLED use interface 0 */
#define PACLED64_LED_INTERFACE      0
#define NANOLED_LED_INTERFACE       0

/* ========================================================================
 * LED Command Bytes
 * ======================================================================== */

/* I-PAC Ultimate I/O command bytes (uses 5-byte messages with report ID 0x03) */
#define IPAC_REPORT_ID              0x03    /* HID Report ID for LED commands */
#define IPAC_CMD_SET_LED            0x00    /* Set single LED: led_index in byte[1], intensity in byte[2] */
#define IPAC_CMD_SET_ALL_LEDS       0x80    /* Set all LEDs: 0x80 in byte[1], intensity in byte[2] */
#define IPAC_CMD_RANDOM             0x89    /* Random LED states: 0x89 in byte[1] */
#define IPAC_CMD_FADE_RATE          0xC0    /* Set fade rate: 0xC0 in byte[1], rate in byte[2] (0-255) */

/* Legacy command byte (used in older protocol, kept for compatibility) */
#define IPAC_CMD_SET_LED_LEGACY     0xDD    /* Set single LED intensity (5-byte direct) */

/* PacLED64/NanoLED command bytes (64-LED protocol, 2-byte messages) */
#define PAC64_CMD_SET_LED           0x80    /* Set single LED: 0x80 | (led_index & 0x3F) */
#define PAC64_CMD_FADE_TIME         0x40    /* Set fade time */
#define PAC64_CMD_RANDOM            0x89    /* Random LED states */
#define PAC64_CMD_SET_ALL           0x00    /* Set all LEDs (with intensity array) */
#define PAC64_CMD_FLASH_SPEED       0xC0    /* Set flash speed: 0xC0 | (led_index & 0x3F) */
#define PAC64_CMD_FLASH_ALL         0x41    /* Set flash speed for all LEDs */
#define PAC64_CMD_SCRIPT_START      0x42    /* Start script recording */
#define PAC64_CMD_SCRIPT_STOP       0x43    /* Stop script recording */
#define PAC64_CMD_SCRIPT_DELAY      0x44    /* Set script step delay */
#define PAC64_CMD_SCRIPT_RUN        0x45    /* Run recorded script */
#define PAC64_CMD_CLEAR_FLASH       0x46    /* Clear flash memory */
#define PAC64_CMD_SET_DEVICE_ID     0xFE    /* Set device ID */
#define PAC64_CMD_UPDATE_FIRMWARE   0xFF    /* Update firmware */

/* PacDrive (16-LED) command byte */
#define PACDRIVE_CMD_SET_LEDS       0x00    /* Set LED states (bitmask) */

/* ServoStik command bytes */
#define SERVOSTIK_CMD_4WAY          0x00    /* Set 4-way mode */
#define SERVOSTIK_CMD_8WAY          0xFF    /* Set 8-way mode */

/* USB Button command bytes */
#define USBBUTTON_CMD_PERMANENT     0x01    /* Configure permanent settings */
#define USBBUTTON_CMD_TEMPORARY     0x02    /* Configure temporary settings */
#define USBBUTTON_CMD_COLOR         0x03    /* Set RGB color */

/* ========================================================================
 * Device Type Enumeration
 * ======================================================================== */

typedef enum {
    DEVICE_UNKNOWN = 0,
    DEVICE_PACDRIVE,            /* 16 LEDs, on/off only */
    DEVICE_UHID,                /* 16 LEDs, on/off only */
    DEVICE_NANOLED,             /* 60 LEDs, 256 intensity levels */
    DEVICE_PACLED64,            /* 64 LEDs, 256 intensity levels */
    DEVICE_IPAC_ULTIMATE_IO,    /* 96 LEDs, 256 intensity levels */
    DEVICE_SERVOSTIK,           /* Joystick restrictor plate */
    DEVICE_USBBUTTON,           /* RGB illuminated button */
    DEVICE_TYPE_COUNT
} UltimarcDeviceType;

/* ========================================================================
 * Device Capabilities
 * ======================================================================== */

typedef struct {
    UltimarcDeviceType type;
    const char *name;           /* Human-readable device name */
    int led_count;              /* Total number of LED channels */
    int rgb_led_count;          /* Number of RGB LED groups (led_count/3 for PWM devices) */
    bool has_intensity;         /* Supports 256 brightness levels */
    bool has_fade;              /* Supports hardware fade */
    bool has_flash;             /* Supports hardware flash */
    bool has_scripting;         /* Supports hardware scripting */
    int usb_interface;          /* USB interface for LED control */
} UltimarcDeviceInfo;

/* ========================================================================
 * Function Prototypes
 * ======================================================================== */

/**
 * Get device type from vendor ID and product ID
 * @param vid Vendor ID
 * @param pid Product ID
 * @return Device type, or DEVICE_UNKNOWN if not recognized
 */
UltimarcDeviceType ultimarc_detect_device_type(uint16_t vid, uint16_t pid);

/**
 * Get device information for a device type
 * @param type Device type
 * @return Pointer to device info (static, do not free), or NULL if unknown
 */
const UltimarcDeviceInfo *ultimarc_get_device_info(UltimarcDeviceType type);

/**
 * Get human-readable name for a device type
 * @param type Device type
 * @return Device name string
 */
const char *ultimarc_device_type_name(UltimarcDeviceType type);

/**
 * Check if a vendor/product ID combo is an Ultimarc LED controller
 * @param vid Vendor ID
 * @param pid Product ID
 * @return true if the device is a supported LED controller
 */
bool ultimarc_is_led_controller(uint16_t vid, uint16_t pid);

/**
 * Get device index from product ID (for devices that support multiple units)
 * @param type Device type
 * @param pid Product ID
 * @return Device index (0-based), or 0 for single-device products
 */
int ultimarc_get_device_index(UltimarcDeviceType type, uint16_t pid);

#endif /* ULTIMARC_H */
