#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "retropac.h"

/* 
 * Ultimarc I-PAC Ultimate I/O LED control implementation
 * 
 * The I-PAC Ultimate I/O uses USB HID for LED control.
 * LED messages are sent via HID Output Reports.
 * 
 * Interface selection:
 * - Interface 2: Non-Game-Controller mode (NGC)
 * - Interface 3: Game Controller mode
 * 
 * Message format for setting LEDs (5 bytes):
 * Byte 0: 0x03 (Report ID)
 * Byte 1: LED index (1-96, 1-based) or command byte
 * Byte 2: Intensity (0-255) or parameter
 * Bytes 3-4: 0x00 (padding)
 * 
 * wValue: 0x0203 (Report Type: Output [0x02], Report ID: 3 [0x03])
 * 
 * Special commands:
 * - LED index 0x80 (128): Set all LEDs to same intensity
 * - LED index 0x89 (137): Random LED states
 * - LED index 0xC0 (192): Set fade rate (0=instant, 1-255=slower)
 * 
 * Pin mappings are loaded from the configuration file.
 */

#define IPAC_LED_REPORT_SIZE 5
#define IPAC_LED_INTERFACE 2  /* Vendor-specific interface for LED control */

/* Track if libusb has been initialized (reference counting for multi-controller) */
static int libusb_init_count = 0;

/* Initialize connection to i-pac controller */
int ipac_init(IpacController *controller) {
    libusb_device_handle *handle = NULL;
    int result;
    
    /* Initialize libusb if not already done */
    if (libusb_init_count == 0) {
        result = libusb_init(NULL);
        if (result < 0) {
            fprintf(stderr, "Error: libusb_init failed: %s\n", libusb_error_name(result));
            return -1;
        }
    }
    libusb_init_count++;
    
    handle = libusb_open_device_with_vid_pid(NULL, controller->vendor_id, controller->product_id);
    if (!handle) {
        fprintf(stderr, "Error: Could not find i-pac device (VID: 0x%04x, PID: 0x%04x)\n",
                controller->vendor_id, controller->product_id);
        fprintf(stderr, "Make sure the device is connected and you have permissions.\n");
        fprintf(stderr, "You may need to run as root or add udev rules.\n");
        libusb_init_count--;
        if (libusb_init_count == 0) {
            libusb_exit(NULL);
        }
        return -1;
    }
    
    /* Only detach kernel driver from interface 2 (LED control) 
     * Leave interfaces 0 and 1 alone - they handle joystick/button input
     * and we want those to keep working while we control LEDs */
    controller->driver_detached = 0;
    if (libusb_kernel_driver_active(handle, IPAC_LED_INTERFACE) == 1) {
        result = libusb_detach_kernel_driver(handle, IPAC_LED_INTERFACE);
        if (result < 0) {
            fprintf(stderr, "Warning: Could not detach kernel driver from interface %d: %s\n", 
                    IPAC_LED_INTERFACE, libusb_error_name(result));
        } else {
            controller->driver_detached = 1;
        }
    }
    
    /* Claim interface 2 (vendor-specific LED interface) */
    result = libusb_claim_interface(handle, IPAC_LED_INTERFACE);
    if (result < 0) {
        fprintf(stderr, "Error: Could not claim LED interface %d: %s\n", 
                IPAC_LED_INTERFACE, libusb_error_name(result));
        /* Reattach kernel driver if we detached it */
        if (controller->driver_detached) {
            libusb_attach_kernel_driver(handle, IPAC_LED_INTERFACE);
            controller->driver_detached = 0;
        }
        libusb_close(handle);
        libusb_init_count--;
        if (libusb_init_count == 0) {
            libusb_exit(NULL);
        }
        return -1;
    }
    controller->claimed_interface = IPAC_LED_INTERFACE;
    controller->usb_handle = handle;
    
    printf("Successfully connected to %s (VID: 0x%04x, PID: 0x%04x) on interface %d\n",
           controller->device_name, controller->vendor_id, controller->product_id, 
           controller->claimed_interface);
    
    return 0;
}

/* Send LED intensity for a single pin */
static int send_led_command(libusb_device_handle *handle, int claimed_interface, 
                            uint8_t led_index, uint8_t intensity) {
    unsigned char data[5];
    int result;
    
    /*
     * Ultimarc iPAC Ultimate I/O LED Protocol:
     * 
     * LEDs are controlled via HID Output Reports on interface 2 (or 3 for Game Controller mode).
     * 
     * Message format (5 bytes):
     * Byte 0: 0x03 (Report ID)
     * Byte 1: LED index (1-96, 1-based)
     * Byte 2: Intensity (0-255)
     * Byte 3-4: 0x00 (padding)
     * 
     * wValue = 0x0203 (Report Type: Output [0x02], Report ID: 3 [0x03])
     */
    
    /* LED indices are 0-based (0-95), config uses 1-based pin numbers (1-96) */
    uint8_t led_idx = (led_index > 0) ? led_index - 1 : 0;
    
    memset(data, 0, sizeof(data));
    data[0] = 0x03;         /* Report ID */
    data[1] = led_idx;      /* LED index (0-95, 0-based) */
    data[2] = intensity;    /* Intensity (0-255) */
    
    result = libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0203, /* Report Type: Output, Report ID: 3 */
        claimed_interface,
        data,
        5,
        1000
    );
    
    return result;
}

/* Set LED color for a single button */
int ipac_set_led(IpacController *controller, ButtonType button, RGBColor color) {
    int result;
    
    if (!controller || !controller->usb_handle || !controller->pin_mappings) {
        return -1;
    }
    
    if (button >= BUTTON_MAX) {
        return -1;
    }
    
    libusb_device_handle *dev_handle = (libusb_device_handle *)controller->usb_handle;
    PinMapping pins = controller->pin_mappings[button];
    
    /* Check if pins are configured - silently skip unconfigured buttons */
    if (pins.r_pin < 0 || pins.g_pin < 0 || pins.b_pin < 0) {
        return 0;  /* Not an error, just not configured */
    }
    
    /* Apply gamma correction for more natural brightness perception */
    RGBColor corrected = gamma_correct_color(color);
    
    /* Set red channel */
    result = send_led_command(dev_handle, controller->claimed_interface, pins.r_pin, corrected.r);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set red LED (pin %d): %s\n", 
                pins.r_pin, libusb_error_name(result));
    }
    
    /* Set green channel */
    result = send_led_command(dev_handle, controller->claimed_interface, pins.g_pin, corrected.g);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set green LED (pin %d): %s\n", 
                pins.g_pin, libusb_error_name(result));
    }
    
    /* Set blue channel */
    result = send_led_command(dev_handle, controller->claimed_interface, pins.b_pin, corrected.b);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set blue LED (pin %d): %s\n", 
                pins.b_pin, libusb_error_name(result));
        return -1;
    }
    
    return 0;
}

/* Turn off all configured LEDs */
int ipac_clear_all_leds(IpacController *controller) {
    if (!controller || !controller->usb_handle || !controller->pin_mappings) {
        return -1;
    }
    
    libusb_device_handle *dev_handle = (libusb_device_handle *)controller->usb_handle;
    int cleared = 0;
    
    printf("Clearing all LEDs on %s...\n", controller->device_name);
    
    /* Iterate through all possible buttons and turn off any that have pin mappings */
    for (int button = 0; button < BUTTON_MAX; button++) {
        PinMapping pins = controller->pin_mappings[button];
        
        /* Only clear if pins are configured */
        if (pins.r_pin >= 0 && pins.g_pin >= 0 && pins.b_pin >= 0) {
            send_led_command(dev_handle, controller->claimed_interface, pins.r_pin, 0);
            send_led_command(dev_handle, controller->claimed_interface, pins.g_pin, 0);
            send_led_command(dev_handle, controller->claimed_interface, pins.b_pin, 0);
            cleared++;
        }
    }
    
    printf("Cleared %d button LEDs\n", cleared);
    return 0;
}

/* Set all LEDs based on button configuration array */
int ipac_set_all_leds(IpacController *controller, ButtonConfig *buttons, int count) {
    if (!controller || !controller->usb_handle) {
        return -1;
    }
    
    int success = 0;
    int failed = 0;
    
    /* Clear all LEDs first for a clean transition */
    ipac_clear_all_leds(controller);
    
    printf("Setting %d button LEDs on %s...\n", count, controller->device_name);
    
    for (int i = 0; i < count; i++) {
        printf("  %s -> RGB(%d, %d, %d)\n",
               button_enum_to_name(buttons[i].button),
               buttons[i].color.r,
               buttons[i].color.g,
               buttons[i].color.b);
        
        if (ipac_set_led(controller, buttons[i].button, buttons[i].color) == 0) {
            success++;
        } else {
            failed++;
        }
    }
    
    printf("LED update complete: %d successful, %d failed\n", success, failed);
    
    return (failed == 0) ? 0 : -1;
}

/* Close connection to i-pac controller */
void ipac_close(IpacController *controller) {
    if (!controller || !controller->usb_handle) {
        return;
    }
    
    libusb_device_handle *dev_handle = (libusb_device_handle *)controller->usb_handle;
    
    if (controller->claimed_interface >= 0) {
        libusb_release_interface(dev_handle, controller->claimed_interface);
    }
    
    /* Reattach kernel driver for LED interface if we detached it */
    if (controller->driver_detached) {
        int result = libusb_attach_kernel_driver(dev_handle, IPAC_LED_INTERFACE);
        if (result < 0) {
            fprintf(stderr, "Warning: Could not reattach kernel driver to interface %d: %s\n",
                    IPAC_LED_INTERFACE, libusb_error_name(result));
        }
        controller->driver_detached = 0;
    }
    
    libusb_close(dev_handle);
    controller->usb_handle = NULL;
    controller->claimed_interface = -1;
    
    printf("Disconnected from %s\n", controller->device_name);
    
    /* Decrement reference count and exit libusb if no more controllers */
    libusb_init_count--;
    if (libusb_init_count == 0) {
        libusb_exit(NULL);
    }
}

/* Initialize all controllers */
int ipac_init_all(IpacController *controllers, int count) {
    int success = 0;
    
    for (int i = 0; i < count; i++) {
        if (ipac_init(&controllers[i]) == 0) {
            success++;
        }
    }
    
    return success;
}

/* Close all controllers */
void ipac_close_all(IpacController *controllers, int count) {
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            ipac_close(&controllers[i]);
        }
    }
}

/* Set LED on all controllers that have the button mapped */
int ipac_set_led_all(IpacController *controllers, int count, ButtonType button, RGBColor color) {
    int success = 0;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            if (ipac_set_led(&controllers[i], button, color) == 0) {
                success++;
            }
        }
    }
    
    return success > 0 ? 0 : -1;
}

/* Clear all LEDs on all controllers */
int ipac_clear_all_leds_all(IpacController *controllers, int count) {
    int success = 0;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            if (ipac_clear_all_leds(&controllers[i]) == 0) {
                success++;
            }
        }
    }
    
    return success > 0 ? 0 : -1;
}

/* Set all LEDs on all controllers */
int ipac_set_all_leds_all(IpacController *controllers, int count, ButtonConfig *buttons, int btn_count) {
    int success = 0;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            if (ipac_set_all_leds(&controllers[i], buttons, btn_count) == 0) {
                success++;
            }
        }
    }
    
    return success > 0 ? 0 : -1;
}

/* Find controller by device name */
IpacController *ipac_find_controller_by_name(IpacController *controllers, int count, const char *name) {
    if (!controllers || !name) return NULL;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].device_name && strcmp(controllers[i].device_name, name) == 0) {
            return &controllers[i];
        }
    }
    
    return NULL;
}

/* Apply ROM configuration with per-controller button configs */
int ipac_apply_rom_config(IpacController *controllers, int controller_count, 
                          ControllerButtonConfig *ctrl_configs, int config_count) {
    if (!controllers || controller_count == 0 || !ctrl_configs || config_count == 0) {
        return -1;
    }
    
    int success = 0;
    
    /* First, clear all LEDs on all controllers for a clean state */
    ipac_clear_all_leds_all(controllers, controller_count);
    
    /* Apply each controller-specific configuration */
    for (int i = 0; i < config_count; i++) {
        ControllerButtonConfig *cfg = &ctrl_configs[i];
        
        if (!cfg->controller_name || cfg->button_count == 0) {
            continue;
        }
        
        /* Check for wildcard "*" - applies to all controllers (legacy mode) */
        if (strcmp(cfg->controller_name, "*") == 0) {
            printf("Applying configuration to all controllers (%d buttons)\n", cfg->button_count);
            for (int c = 0; c < controller_count; c++) {
                if (controllers[c].usb_handle) {
                    printf("  Sending to %s...\n", controllers[c].device_name);
                    if (ipac_set_all_leds(&controllers[c], cfg->buttons, cfg->button_count) == 0) {
                        success++;
                    }
                }
            }
        } else {
            /* Find the specific controller by name */
            IpacController *target = ipac_find_controller_by_name(controllers, controller_count, 
                                                                   cfg->controller_name);
            if (target && target->usb_handle) {
                printf("Applying configuration to %s (%d buttons)\n", 
                       cfg->controller_name, cfg->button_count);
                if (ipac_set_all_leds(target, cfg->buttons, cfg->button_count) == 0) {
                    success++;
                }
            } else {
                fprintf(stderr, "Warning: Controller '%s' not found or not connected\n", 
                        cfg->controller_name);
            }
        }
    }
    
    return success > 0 ? 0 : -1;
}
