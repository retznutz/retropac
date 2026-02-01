/**
 * @file ultimarc.c
 * @brief Ultimarc device detection and information
 * 
 * Provides device type detection and capability information for all
 * supported Ultimarc LED controllers.
 */

#include <stdio.h>
#include <string.h>
#include "ultimarc.h"

/* ========================================================================
 * Static Device Information Table
 * ======================================================================== */

static const UltimarcDeviceInfo device_info_table[] = {
    /* DEVICE_UNKNOWN */
    {
        .type = DEVICE_UNKNOWN,
        .name = "Unknown",
        .led_count = 0,
        .rgb_led_count = 0,
        .has_intensity = false,
        .has_fade = false,
        .has_flash = false,
        .has_scripting = false,
        .usb_interface = 0
    },
    /* DEVICE_PACDRIVE */
    {
        .type = DEVICE_PACDRIVE,
        .name = "PacDrive",
        .led_count = LED_COUNT_PACDRIVE,
        .rgb_led_count = 5,  /* 16 LEDs / 3 = 5 RGB LEDs */
        .has_intensity = false,
        .has_fade = false,
        .has_flash = false,
        .has_scripting = false,
        .usb_interface = 0
    },
    /* DEVICE_UHID */
    {
        .type = DEVICE_UHID,
        .name = "U-HID",
        .led_count = LED_COUNT_UHID,
        .rgb_led_count = 5,  /* 16 LEDs / 3 = 5 RGB LEDs */
        .has_intensity = false,
        .has_fade = false,
        .has_flash = false,
        .has_scripting = false,
        .usb_interface = 0
    },
    /* DEVICE_NANOLED */
    {
        .type = DEVICE_NANOLED,
        .name = "NanoLED",
        .led_count = LED_COUNT_NANOLED,
        .rgb_led_count = RGB_COUNT_NANOLED,
        .has_intensity = true,
        .has_fade = true,
        .has_flash = true,
        .has_scripting = true,
        .usb_interface = NANOLED_LED_INTERFACE
    },
    /* DEVICE_PACLED64 */
    {
        .type = DEVICE_PACLED64,
        .name = "PacLED64",
        .led_count = LED_COUNT_PACLED64,
        .rgb_led_count = RGB_COUNT_PACLED64,
        .has_intensity = true,
        .has_fade = true,
        .has_flash = true,
        .has_scripting = true,
        .usb_interface = PACLED64_LED_INTERFACE
    },
    /* DEVICE_IPAC_ULTIMATE_IO */
    {
        .type = DEVICE_IPAC_ULTIMATE_IO,
        .name = "I-PAC Ultimate I/O",
        .led_count = LED_COUNT_IPAC_ULTIMATE_IO,
        .rgb_led_count = RGB_COUNT_IPAC_ULTIMATE_IO,
        .has_intensity = true,
        .has_fade = true,
        .has_flash = true,
        .has_scripting = false,  /* I-PAC Ultimate doesn't have scripting */
        .usb_interface = IPAC_ULTIMATE_LED_INTERFACE
    },
    /* DEVICE_SERVOSTIK */
    {
        .type = DEVICE_SERVOSTIK,
        .name = "ServoStik",
        .led_count = 0,
        .rgb_led_count = 0,
        .has_intensity = false,
        .has_fade = false,
        .has_flash = false,
        .has_scripting = false,
        .usb_interface = 0
    },
    /* DEVICE_USBBUTTON */
    {
        .type = DEVICE_USBBUTTON,
        .name = "USB Button",
        .led_count = 3,  /* R, G, B channels */
        .rgb_led_count = 1,
        .has_intensity = true,
        .has_fade = false,
        .has_flash = false,
        .has_scripting = false,
        .usb_interface = 0
    }
};

/* ========================================================================
 * Device Detection
 * ======================================================================== */

UltimarcDeviceType ultimarc_detect_device_type(uint16_t vid, uint16_t pid) {
    if (!IS_ULTIMARC_VID(vid)) {
        return DEVICE_UNKNOWN;
    }
    
    if (IS_PACDRIVE(pid)) {
        return DEVICE_PACDRIVE;
    }
    if (IS_UHID(pid)) {
        return DEVICE_UHID;
    }
    if (IS_NANOLED(pid)) {
        return DEVICE_NANOLED;
    }
    if (IS_PACLED64(pid)) {
        return DEVICE_PACLED64;
    }
    if (IS_IPAC_ULTIMATE_IO(pid)) {
        return DEVICE_IPAC_ULTIMATE_IO;
    }
    if (IS_SERVOSTIK(pid)) {
        return DEVICE_SERVOSTIK;
    }
    if (IS_USBBUTTON(pid)) {
        return DEVICE_USBBUTTON;
    }
    
    return DEVICE_UNKNOWN;
}

/* ========================================================================
 * Device Information
 * ======================================================================== */

const UltimarcDeviceInfo *ultimarc_get_device_info(UltimarcDeviceType type) {
    if (type < 0 || type >= DEVICE_TYPE_COUNT) {
        return &device_info_table[DEVICE_UNKNOWN];
    }
    return &device_info_table[type];
}

const char *ultimarc_device_type_name(UltimarcDeviceType type) {
    const UltimarcDeviceInfo *info = ultimarc_get_device_info(type);
    return info ? info->name : "Unknown";
}

bool ultimarc_is_led_controller(uint16_t vid, uint16_t pid) {
    UltimarcDeviceType type = ultimarc_detect_device_type(vid, pid);
    
    switch (type) {
        case DEVICE_PACDRIVE:
        case DEVICE_UHID:
        case DEVICE_NANOLED:
        case DEVICE_PACLED64:
        case DEVICE_IPAC_ULTIMATE_IO:
        case DEVICE_USBBUTTON:
            return true;
        default:
            return false;
    }
}

int ultimarc_get_device_index(UltimarcDeviceType type, uint16_t pid) {
    switch (type) {
        case DEVICE_UHID:
            return pid - PID_UHID_LO;
        case DEVICE_NANOLED:
            return pid - PID_NANOLED_LO;
        case DEVICE_PACLED64:
            return pid - PID_PACLED64_LO;
        case DEVICE_IPAC_ULTIMATE_IO:
            return pid - PID_IPAC_ULTIMATE_IO_LO;
        default:
            return 0;  /* Single-device products */
    }
}
