#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "retropac.h"

/* 
 * Ultimarc i-pac Ultimate I/O LED control implementation
 * 
 * The iPAC Ultimate I/O uses USB HID for LED control.
 * LED messages are sent via HID Output Reports to interface 2.
 * 
 * Message format for setting LEDs:
 * Byte 0: 0x03 (Report ID / Header)
 * Byte 1-4: LED data pairs [LED_index, intensity] or command data
 * 
 * Pin mappings are loaded from the configuration file.
 */

#define IPAC_LED_REPORT_SIZE 5
#define IPAC_LED_INTERFACE 2  /* Vendor-specific interface for LED control */

/* Store the interface we claimed for cleanup */
static int claimed_interface = -1;

/* Initialize connection to i-pac controller */
int ipac_init(IpacController *controller) {
    libusb_device_handle *handle = NULL;
    int result;
    
    result = libusb_init(NULL);
    if (result < 0) {
        fprintf(stderr, "Error: libusb_init failed: %s\n", libusb_error_name(result));
        return -1;
    }
    
    handle = libusb_open_device_with_vid_pid(NULL, controller->vendor_id, controller->product_id);
    if (!handle) {
        fprintf(stderr, "Error: Could not find i-pac device (VID: 0x%04x, PID: 0x%04x)\n",
                controller->vendor_id, controller->product_id);
        fprintf(stderr, "Make sure the device is connected and you have permissions.\n");
        fprintf(stderr, "You may need to run as root or add udev rules.\n");
        libusb_exit(NULL);
        return -1;
    }
    
    /* Detach kernel drivers from all interfaces we might need */
    for (int iface = 0; iface <= 2; iface++) {
        if (libusb_kernel_driver_active(handle, iface) == 1) {
            result = libusb_detach_kernel_driver(handle, iface);
            if (result < 0) {
                fprintf(stderr, "Warning: Could not detach kernel driver from interface %d: %s\n", 
                        iface, libusb_error_name(result));
            }
        }
    }
    
    /* Claim interface 2 (vendor-specific LED interface) */
    result = libusb_claim_interface(handle, IPAC_LED_INTERFACE);
    if (result < 0) {
        fprintf(stderr, "Warning: Could not claim interface %d: %s\n", 
                IPAC_LED_INTERFACE, libusb_error_name(result));
        /* Fall back to interface 0 */
        result = libusb_claim_interface(handle, 0);
        if (result < 0) {
            fprintf(stderr, "Error: Could not claim interface 0: %s\n", libusb_error_name(result));
            libusb_close(handle);
            libusb_exit(NULL);
            return -1;
        }
        claimed_interface = 0;
    } else {
        claimed_interface = IPAC_LED_INTERFACE;
    }
    
    printf("Successfully connected to %s (VID: 0x%04x, PID: 0x%04x) on interface %d\n",
           controller->device_name, controller->vendor_id, controller->product_id, claimed_interface);
    
    return (int)(intptr_t)handle;
}

/* Send LED intensity for a single pin */
static int send_led_command(libusb_device_handle *handle, uint8_t led_index, uint8_t intensity) {
    unsigned char data[5];
    int result;
    
    /*
     * Ultimarc iPAC Ultimate I/O LED Protocol:
     * 
     * LEDs are controlled via HID Feature Reports on interface 2.
     * 
     * Message format:
     * Byte 0: 0xDD (Set LED intensity command)
     * Byte 1: LED index (0-based, 0-95)
     * Byte 2: Intensity (0-255)
     * Byte 3-4: 0x00 (padding)
     */
    
    /* LED indices are 0-based, config uses 1-based pin numbers */
    uint8_t led_idx = (led_index > 0) ? led_index - 1 : 0;
    
    memset(data, 0, sizeof(data));
    data[0] = 0xDD;         /* Set LED command */
    data[1] = led_idx;      /* LED index (0-95) */
    data[2] = intensity;    /* Intensity (0-255) */
    
    result = libusb_control_transfer(
        handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,   /* HID Set_Report */
        0x0300, /* Report Type: Feature, Report ID: 0 */
        claimed_interface,
        data,
        5,
        1000
    );
    
    return result;
}

/* Set LED color for a single button */
int ipac_set_led(int handle, ButtonType button, RGBColor color, PinMapping *pin_mappings) {
    int result;
    libusb_device_handle *dev_handle = (libusb_device_handle *)(intptr_t)handle;
    
    if (button >= BUTTON_MAX || !pin_mappings) {
        fprintf(stderr, "Error: Invalid button type or pin mappings\n");
        return -1;
    }
    
    PinMapping pins = pin_mappings[button];
    
    /* Check if pins are configured */
    if (pins.r_pin < 0 || pins.g_pin < 0 || pins.b_pin < 0) {
        fprintf(stderr, "Warning: Button %s has no pin mapping configured\n", 
                button_enum_to_name(button));
        return -1;
    }
    
    /* Set red channel */
    result = send_led_command(dev_handle, pins.r_pin, color.r);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set red LED (pin %d): %s\n", 
                pins.r_pin, libusb_error_name(result));
    }
    
    /* Set green channel */
    result = send_led_command(dev_handle, pins.g_pin, color.g);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set green LED (pin %d): %s\n", 
                pins.g_pin, libusb_error_name(result));
    }
    
    /* Set blue channel */
    result = send_led_command(dev_handle, pins.b_pin, color.b);
    if (result < 0) {
        fprintf(stderr, "Warning: Failed to set blue LED (pin %d): %s\n", 
                pins.b_pin, libusb_error_name(result));
        return -1;
    }
    
    return 0;
}

/* Turn off all configured LEDs */
int ipac_clear_all_leds(int handle, PinMapping *pin_mappings) {
    libusb_device_handle *dev_handle = (libusb_device_handle *)(intptr_t)handle;
    RGBColor off = {0, 0, 0};
    int cleared = 0;
    
    printf("Clearing all LEDs...\n");
    
    /* Iterate through all possible buttons and turn off any that have pin mappings */
    for (int button = 0; button < BUTTON_MAX; button++) {
        PinMapping pins = pin_mappings[button];
        
        /* Only clear if pins are configured */
        if (pins.r_pin >= 0 && pins.g_pin >= 0 && pins.b_pin >= 0) {
            send_led_command(dev_handle, pins.r_pin, off.r);
            send_led_command(dev_handle, pins.g_pin, off.g);
            send_led_command(dev_handle, pins.b_pin, off.b);
            cleared++;
        }
    }
    
    printf("Cleared %d button LEDs\n", cleared);
    return 0;
}

/* Set all LEDs based on button configuration array */
int ipac_set_all_leds(int handle, ButtonConfig *buttons, int count, PinMapping *pin_mappings) {
    int success = 0;
    int failed = 0;
    
    /* Clear all LEDs first for a clean transition */
    ipac_clear_all_leds(handle, pin_mappings);
    
    printf("Setting %d button LEDs...\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("  %s -> RGB(%d, %d, %d)\n",
               button_enum_to_name(buttons[i].button),
               buttons[i].color.r,
               buttons[i].color.g,
               buttons[i].color.b);
        
        if (ipac_set_led(handle, buttons[i].button, buttons[i].color, pin_mappings) == 0) {
            success++;
        } else {
            failed++;
        }
    }
    
    printf("LED update complete: %d successful, %d failed\n", success, failed);
    
    return (failed == 0) ? 0 : -1;
}

/* Close connection to i-pac controller */
void ipac_close(int handle) {
    if (handle > 0) {
        libusb_device_handle *dev_handle = (libusb_device_handle *)(intptr_t)handle;
        if (claimed_interface >= 0) {
            libusb_release_interface(dev_handle, claimed_interface);
        }
        libusb_close(dev_handle);
        libusb_exit(NULL);
        claimed_interface = -1;
        printf("Disconnected from i-pac controller\n");
    }
}
