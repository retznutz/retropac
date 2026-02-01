/**
 * @file pac_led.c
 * @brief Unified LED control for all Ultimarc devices
 * 
 * This module provides a unified API for controlling LEDs across all
 * supported Ultimarc LED controller devices. It automatically detects
 * the device type and uses the appropriate protocol.
 * 
 * Supported devices:
 * - PacDrive (16 LEDs, on/off only)
 * - U-HID (16 LEDs, on/off only)
 * - NanoLED (60 LEDs, 256 intensity levels)
 * - PacLED64 (64 LEDs, 256 intensity levels)
 * - I-PAC Ultimate I/O (96 LEDs, 256 intensity levels)
 * - USB Button (single RGB LED)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "ultimarc.h"
#include "retropac.h"

/* Track libusb initialization */
static int libusb_init_count = 0;

/* ========================================================================
 * Internal: Protocol-specific LED commands
 * ======================================================================== */

/**
 * Send LED command for I-PAC Ultimate I/O
 * Uses HID control transfers on interface 2 (or 3 for non-game-controller mode)
 * 
 * Protocol: 5-byte message
 * - Byte 0: Report ID (0x03)
 * - Byte 1: LED index (0-95) or command
 * - Byte 2: Intensity (0-255) or parameter
 * - Bytes 3-4: Padding (0x00)
 */
static int send_ipac_ultimate_cmd(libusb_device_handle *handle, int interface,
                                   uint8_t cmd_or_led, uint8_t value) {
    unsigned char data[5];
    
    memset(data, 0, sizeof(data));
    data[0] = IPAC_REPORT_ID;   /* Report ID */
    data[1] = cmd_or_led;       /* LED index or command */
    data[2] = value;            /* Intensity or parameter */
    
    return libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0203, /* Report Type: Output, Report ID: 3 */
        interface,
        data,
        5,
        1000
    );
}

/**
 * Send LED intensity for I-PAC Ultimate I/O (convenience wrapper)
 */
static int send_ipac_ultimate_led(libusb_device_handle *handle, int interface,
                                   uint8_t led_index, uint8_t intensity) {
    return send_ipac_ultimate_cmd(handle, interface, led_index, intensity);
}

/**
 * Send fade rate command for I-PAC Ultimate I/O
 * Rate 0 = instant, 1-255 = increasingly slow fade
 */
static int send_ipac_ultimate_fade_rate(libusb_device_handle *handle, int interface,
                                         uint8_t fade_rate) {
    return send_ipac_ultimate_cmd(handle, interface, IPAC_CMD_FADE_RATE, fade_rate);
}

/**
 * Set all LEDs to same intensity on I-PAC Ultimate I/O
 */
static int send_ipac_ultimate_all(libusb_device_handle *handle, int interface,
                                   uint8_t intensity) {
    return send_ipac_ultimate_cmd(handle, interface, IPAC_CMD_SET_ALL_LEDS, intensity);
}

/**
 * Set random LED states on I-PAC Ultimate I/O
 */
static int send_ipac_ultimate_random(libusb_device_handle *handle, int interface) {
    return send_ipac_ultimate_cmd(handle, interface, IPAC_CMD_RANDOM, 0);
}

/**
 * Send LED command for PacLED64/NanoLED (64-LED protocol)
 * Uses HID control transfers (not interrupt as previously thought)
 * 
 * Protocol: 2-byte message
 * - Byte 0: Command | LED index
 * - Byte 1: Value (intensity, fade time, etc.)
 */
static int send_pac64_cmd(libusb_device_handle *handle, int interface,
                          uint8_t cmd, uint8_t value) {
    unsigned char data[2];
    
    data[0] = cmd;
    data[1] = value;
    
    return libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0200, /* Report Type: Output */
        interface,
        data,
        2,
        1000
    );
}

/**
 * Send LED intensity for PacLED64/NanoLED
 */
static int send_pac64_led(libusb_device_handle *handle, int interface,
                          uint8_t led_index, uint8_t intensity) {
    /* LED index is encoded in lower 6 bits of command byte */
    /* For LEDs 0-63: use direct index */
    /* For LED intensity: command is just the LED index for individual LEDs */
    return send_pac64_cmd(handle, interface, led_index, intensity);
}

/**
 * Send fade time for PacLED64/NanoLED
 * This is a GLOBAL setting affecting all subsequent LED changes
 */
static int send_pac64_fade_time(libusb_device_handle *handle, int interface,
                                 uint8_t fade_time) {
    return send_pac64_cmd(handle, interface, PAC64_CMD_FADE_TIME, fade_time);
}

/**
 * Send LED command for PacDrive/U-HID (16-LED protocol)
 * Uses a 16-bit bitmask for on/off state
 */
static int send_pacdrive_leds(libusb_device_handle *handle, int interface,
                               uint16_t led_states) {
    unsigned char data[4];
    
    memset(data, 0, sizeof(data));
    data[0] = PACDRIVE_CMD_SET_LEDS;
    data[1] = (led_states >> 8) & 0xFF;
    data[2] = led_states & 0xFF;
    
    return libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0200, /* Report Type: Output */
        interface,
        data,
        4,
        1000
    );
}

/**
 * Send color command for USB Button
 */
static int send_usbbutton_color(libusb_device_handle *handle, int interface,
                                 uint8_t red, uint8_t green, uint8_t blue) {
    unsigned char data[5];
    
    memset(data, 0, sizeof(data));
    data[0] = USBBUTTON_CMD_COLOR;
    data[1] = red;
    data[2] = green;
    data[3] = blue;
    
    return libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0200, /* Report Type: Output */
        interface,
        data,
        5,
        1000
    );
}

/* ========================================================================
 * Public API: Device-agnostic LED control
 * ======================================================================== */

/**
 * Initialize connection to a PAC LED controller
 * Automatically detects device type and configures appropriately
 */
int pac_led_init(IpacController *controller) {
    libusb_device_handle *handle = NULL;
    const UltimarcDeviceInfo *device_info;
    UltimarcDeviceType device_type;
    int result;
    int led_interface;
    
    /* Detect device type */
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    device_info = ultimarc_get_device_info(device_type);
    
    if (device_type == DEVICE_UNKNOWN) {
        fprintf(stderr, "Error: Unsupported device (VID: 0x%04x, PID: 0x%04x)\n",
                controller->vendor_id, controller->product_id);
        return -1;
    }
    
    if (!ultimarc_is_led_controller(controller->vendor_id, controller->product_id)) {
        fprintf(stderr, "Error: Device %s is not an LED controller\n", device_info->name);
        return -1;
    }
    
    led_interface = device_info->usb_interface;
    
    /* Initialize libusb if needed */
    if (libusb_init_count == 0) {
        result = libusb_init(NULL);
        if (result < 0) {
            fprintf(stderr, "Error: libusb_init failed: %s\n", libusb_error_name(result));
            return -1;
        }
    }
    libusb_init_count++;
    
    /* Open device */
    handle = libusb_open_device_with_vid_pid(NULL, controller->vendor_id, controller->product_id);
    if (!handle) {
        fprintf(stderr, "Error: Could not find %s device (VID: 0x%04x, PID: 0x%04x)\n",
                device_info->name, controller->vendor_id, controller->product_id);
        fprintf(stderr, "Make sure the device is connected and you have permissions.\n");
        libusb_init_count--;
        if (libusb_init_count == 0) {
            libusb_exit(NULL);
        }
        return -1;
    }
    
    /* Detach kernel driver if active */
    controller->driver_detached = 0;
    if (libusb_kernel_driver_active(handle, led_interface) == 1) {
        result = libusb_detach_kernel_driver(handle, led_interface);
        if (result < 0) {
            fprintf(stderr, "Warning: Could not detach kernel driver from interface %d: %s\n",
                    led_interface, libusb_error_name(result));
        } else {
            controller->driver_detached = 1;
        }
    }
    
    /* Claim interface */
    result = libusb_claim_interface(handle, led_interface);
    if (result < 0) {
        fprintf(stderr, "Error: Could not claim LED interface %d: %s\n",
                led_interface, libusb_error_name(result));
        if (controller->driver_detached) {
            libusb_attach_kernel_driver(handle, led_interface);
            controller->driver_detached = 0;
        }
        libusb_close(handle);
        libusb_init_count--;
        if (libusb_init_count == 0) {
            libusb_exit(NULL);
        }
        return -1;
    }
    
    controller->usb_handle = handle;
    controller->claimed_interface = led_interface;
    
    printf("Connected to %s: %s (VID: 0x%04x, PID: 0x%04x) on interface %d\n",
           device_info->name, controller->device_name,
           controller->vendor_id, controller->product_id, led_interface);
    printf("  LED channels: %d, RGB LEDs: %d, Intensity control: %s\n",
           device_info->led_count, device_info->rgb_led_count,
           device_info->has_intensity ? "Yes" : "No (on/off only)");
    
    return 0;
}

/**
 * Set a single LED intensity (device-agnostic)
 */
int pac_led_set_intensity(IpacController *controller, uint8_t led_index, uint8_t intensity) {
    UltimarcDeviceType device_type;
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            return send_ipac_ultimate_led(handle, controller->claimed_interface,
                                          led_index, intensity);
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            return send_pac64_led(handle, controller->claimed_interface,
                                  led_index, intensity);
        
        case DEVICE_PACDRIVE:
        case DEVICE_UHID:
            /* PacDrive only supports on/off, use threshold */
            /* Individual LED control needs state tracking, handled separately */
            fprintf(stderr, "Warning: PacDrive requires batch LED updates\n");
            return -1;
        
        case DEVICE_USBBUTTON:
            fprintf(stderr, "Warning: USB Button uses RGB color, not single LED\n");
            return -1;
        
        default:
            return -1;
    }
}

/**
 * Set RGB color for a button (device-agnostic)
 */
int pac_led_set_rgb(IpacController *controller, ButtonType button, RGBColor color) {
    UltimarcDeviceType device_type;
    libusb_device_handle *handle;
    PinMapping pins;
    RGBColor corrected;
    int result;
    
    if (!controller || !controller->usb_handle || !controller->pin_mappings) {
        return -1;
    }
    
    if (button >= BUTTON_MAX) {
        return -1;
    }
    
    pins = controller->pin_mappings[button];
    if (pins.r_pin < 0 || pins.g_pin < 0 || pins.b_pin < 0) {
        return 0;  /* Not configured, skip silently */
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    
    /* Apply gamma correction */
    corrected = gamma_correct_color(color);
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            /* I-PAC Ultimate uses 0-based LED indices, config pins are 1-based */
            send_ipac_ultimate_led(handle, controller->claimed_interface,
                                   pins.r_pin - 1, corrected.r);
            send_ipac_ultimate_led(handle, controller->claimed_interface,
                                   pins.g_pin - 1, corrected.g);
            result = send_ipac_ultimate_led(handle, controller->claimed_interface,
                                            pins.b_pin - 1, corrected.b);
            break;
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            /* PacLED64/NanoLED use 0-based LED indices */
            send_pac64_led(handle, controller->claimed_interface,
                           pins.r_pin - 1, corrected.r);
            send_pac64_led(handle, controller->claimed_interface,
                           pins.g_pin - 1, corrected.g);
            result = send_pac64_led(handle, controller->claimed_interface,
                                    pins.b_pin - 1, corrected.b);
            break;
        
        case DEVICE_PACDRIVE:
        case DEVICE_UHID:
            /* PacDrive only supports on/off - use threshold (127) */
            {
                uint16_t states = 0;
                if (corrected.r > 127 && pins.r_pin > 0 && pins.r_pin <= 16)
                    states |= (1 << (pins.r_pin - 1));
                if (corrected.g > 127 && pins.g_pin > 0 && pins.g_pin <= 16)
                    states |= (1 << (pins.g_pin - 1));
                if (corrected.b > 127 && pins.b_pin > 0 && pins.b_pin <= 16)
                    states |= (1 << (pins.b_pin - 1));
                result = send_pacdrive_leds(handle, controller->claimed_interface, states);
            }
            break;
        
        case DEVICE_USBBUTTON:
            result = send_usbbutton_color(handle, controller->claimed_interface,
                                          corrected.r, corrected.g, corrected.b);
            break;
        
        default:
            return -1;
    }
    
    return result < 0 ? -1 : 0;
}

/**
 * Set fade time/rate (for devices that support it)
 * 
 * This sets a GLOBAL fade rate that affects all subsequent LED changes.
 * - I-PAC Ultimate: fade_rate 0 = instant, 1-255 = increasingly slow fade
 * - PacLED64/NanoLED: fade_time 0 = instant, 1-255 = fade duration
 * 
 * Once set, any LED intensity change will smoothly transition over the
 * configured time instead of changing instantly.
 */
int pac_led_set_fade_time(IpacController *controller, uint8_t fade_time) {
    UltimarcDeviceType device_type;
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    
    const UltimarcDeviceInfo *info = ultimarc_get_device_info(device_type);
    if (!info->has_fade) {
        fprintf(stderr, "Warning: %s does not support hardware fade\n", info->name);
        return -1;
    }
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            return send_ipac_ultimate_fade_rate(handle, controller->claimed_interface, fade_time);
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            return send_pac64_fade_time(handle, controller->claimed_interface, fade_time);
        
        default:
            return -1;
    }
}

/**
 * Set flash speed for a single LED (for devices that support it)
 * Only PacLED64 and NanoLED support hardware flash.
 * speed: 0 = off, 1-255 = flash rate (higher = faster)
 */
int pac_led_set_flash_speed(IpacController *controller, uint8_t led_index, uint8_t speed) {
    UltimarcDeviceType device_type;
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    
    const UltimarcDeviceInfo *info = ultimarc_get_device_info(device_type);
    if (!info->has_flash) {
        fprintf(stderr, "Warning: %s does not support hardware flash\n", info->name);
        return -1;
    }
    
    /* Flash is only supported on PacLED64/NanoLED */
    return send_pac64_cmd(handle, controller->claimed_interface,
                          PAC64_CMD_FLASH_SPEED | (led_index & 0x3F), speed);
}

/**
 * Set all LEDs to a single intensity (device-agnostic, efficient)
 * Uses device-specific "set all" commands where available.
 */
int pac_led_set_all_intensity(IpacController *controller, uint8_t intensity) {
    UltimarcDeviceType device_type;
    const UltimarcDeviceInfo *info;
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    info = ultimarc_get_device_info(device_type);
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            return send_ipac_ultimate_all(handle, controller->claimed_interface, intensity);
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            /* PacLED64 uses 0x80 | intensity for "all LEDs" */
            return send_pac64_cmd(handle, controller->claimed_interface, 
                                  PAC64_CMD_SET_LED, intensity);
        
        case DEVICE_PACDRIVE:
        case DEVICE_UHID:
            /* Set all 16 LEDs on or off based on threshold */
            return send_pacdrive_leds(handle, controller->claimed_interface,
                                      intensity > 127 ? 0xFFFF : 0x0000);
        
        default:
            /* Fallback: set each LED individually */
            for (int i = 0; i < info->led_count; i++) {
                pac_led_set_intensity(controller, i, intensity);
            }
            return 0;
    }
}

/**
 * Set random LED states (for devices that support it)
 */
int pac_led_set_random(IpacController *controller) {
    UltimarcDeviceType device_type;
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            return send_ipac_ultimate_random(handle, controller->claimed_interface);
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            return send_pac64_cmd(handle, controller->claimed_interface,
                                  PAC64_CMD_RANDOM, 0);
        
        default:
            fprintf(stderr, "Warning: %s does not support random LED states\n",
                    ultimarc_device_type_name(device_type));
            return -1;
    }
}

/**
 * Clear all LEDs (device-agnostic)
 * Uses efficient "set all" commands where available.
 */
int pac_led_clear_all(IpacController *controller) {
    UltimarcDeviceType device_type;
    const UltimarcDeviceInfo *info;
    libusb_device_handle *handle;
    int result = 0;
    
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    device_type = ultimarc_detect_device_type(controller->vendor_id, controller->product_id);
    info = ultimarc_get_device_info(device_type);
    
    printf("Clearing all LEDs on %s (%s)...\n", controller->device_name, info->name);
    
    switch (device_type) {
        case DEVICE_IPAC_ULTIMATE_IO:
            /* Use efficient "set all" command */
            result = send_ipac_ultimate_all(handle, controller->claimed_interface, 0);
            break;
        
        case DEVICE_PACLED64:
        case DEVICE_NANOLED:
            /* Use efficient "set all" command: 0x80 with intensity 0 */
            result = send_pac64_cmd(handle, controller->claimed_interface, PAC64_CMD_SET_LED, 0);
            break;
        
        case DEVICE_PACDRIVE:
        case DEVICE_UHID:
            result = send_pacdrive_leds(handle, controller->claimed_interface, 0x0000);
            break;
        
        case DEVICE_USBBUTTON:
            result = send_usbbutton_color(handle, controller->claimed_interface, 0, 0, 0);
            break;
        
        default:
            return -1;
    }
    
    return result < 0 ? -1 : 0;
}

/**
 * Close connection to a PAC LED controller
 */
void pac_led_close(IpacController *controller) {
    libusb_device_handle *handle;
    
    if (!controller || !controller->usb_handle) {
        return;
    }
    
    handle = (libusb_device_handle *)controller->usb_handle;
    
    if (controller->claimed_interface >= 0) {
        libusb_release_interface(handle, controller->claimed_interface);
    }
    
    if (controller->driver_detached) {
        libusb_attach_kernel_driver(handle, controller->claimed_interface);
        controller->driver_detached = 0;
    }
    
    libusb_close(handle);
    controller->usb_handle = NULL;
    controller->claimed_interface = -1;
    
    printf("Disconnected from %s\n", controller->device_name);
    
    libusb_init_count--;
    if (libusb_init_count == 0) {
        libusb_exit(NULL);
    }
}

/* ========================================================================
 * Device Discovery
 * ======================================================================== */

/**
 * Scan for all connected Ultimarc LED controllers
 * Returns an array of discovered devices (caller must free)
 */
int pac_led_discover(UltimarcDeviceType *devices, int max_devices) {
    libusb_device **list;
    ssize_t count;
    int found = 0;
    int result;
    
    /* Temporarily init libusb for discovery */
    result = libusb_init(NULL);
    if (result < 0) {
        fprintf(stderr, "Error: libusb_init failed: %s\n", libusb_error_name(result));
        return -1;
    }
    
    count = libusb_get_device_list(NULL, &list);
    if (count < 0) {
        libusb_exit(NULL);
        return -1;
    }
    
    for (ssize_t i = 0; i < count && found < max_devices; i++) {
        struct libusb_device_descriptor desc;
        
        if (libusb_get_device_descriptor(list[i], &desc) == 0) {
            UltimarcDeviceType type = ultimarc_detect_device_type(desc.idVendor, desc.idProduct);
            
            if (type != DEVICE_UNKNOWN && ultimarc_is_led_controller(desc.idVendor, desc.idProduct)) {
                const UltimarcDeviceInfo *info = ultimarc_get_device_info(type);
                printf("Found: %s (VID: 0x%04x, PID: 0x%04x)\n",
                       info->name, desc.idVendor, desc.idProduct);
                devices[found++] = type;
            }
        }
    }
    
    libusb_free_device_list(list, 1);
    libusb_exit(NULL);
    
    return found;
}
