#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "retropac.h"

/* 
 * Ultimarc i-pac LED control implementation
 * 
 * Note: This is a basic implementation based on USB HID communication.
 * The Ultimarc i-pac Ultimate I/O supports LED control via USB HID commands.
 * 
 * For production use, you may need to adjust the protocol based on:
 * - Ultimarc's official documentation
 * - The specific i-pac model being used
 * - Testing with actual hardware
 * 
 * Pin mappings are now loaded from the configuration file, allowing users
 * to customize the wiring of their arcade buttons with RGB LEDs.
 */

#define IPAC_LED_REPORT_SIZE 64

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
    
    /* Detach kernel driver if necessary */
    if (libusb_kernel_driver_active(handle, 0) == 1) {
        result = libusb_detach_kernel_driver(handle, 0);
        if (result < 0) {
            fprintf(stderr, "Warning: Could not detach kernel driver: %s\n", 
                    libusb_error_name(result));
        }
    }
    
    /* Claim interface */
    result = libusb_claim_interface(handle, 0);
    if (result < 0) {
        fprintf(stderr, "Error: Could not claim interface: %s\n", libusb_error_name(result));
        libusb_close(handle);
        libusb_exit(NULL);
        return -1;
    }
    
    printf("Successfully connected to %s (VID: 0x%04x, PID: 0x%04x)\n",
           controller->device_name, controller->vendor_id, controller->product_id);
    
    return (int)(intptr_t)handle;
}

/* Set LED color for a single button */
int ipac_set_led(int handle, ButtonType button, RGBColor color, PinMapping *pin_mappings) {
    unsigned char data[IPAC_LED_REPORT_SIZE];
    int result;
    int success_count = 0;
    
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
    
    /* 
     * LED control packet format (simplified):
     * This is a basic implementation. The actual protocol depends on
     * the i-pac model and firmware version.
     * 
     * We need to set each RGB channel separately as they are on different pins.
     * Byte 0: Report ID (0x03 for LED control on some models)
     * Byte 1: Command (0x01 for set LED)
     * Byte 2: Pin number
     * Byte 3: Value (0-255)
     */
    
    /* Set red channel */
    memset(data, 0, sizeof(data));
    data[0] = 0x03; /* Report ID */
    data[1] = 0x01; /* Set LED command */
    data[2] = pins.r_pin;
    data[3] = color.r;
    
    result = libusb_control_transfer(
        (libusb_device_handle *)(intptr_t)handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09, /* HID Set_Report */
        0x0300, /* Report Type: Feature, Report ID: 3 */
        0,
        data,
        IPAC_LED_REPORT_SIZE,
        5000 /* timeout ms */
    );
    
    if (result >= 0) success_count++;
    
    /* Set green channel */
    memset(data, 0, sizeof(data));
    data[0] = 0x03;
    data[1] = 0x01;
    data[2] = pins.g_pin;
    data[3] = color.g;
    
    result = libusb_control_transfer(
        (libusb_device_handle *)(intptr_t)handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,
        0x0300,
        0,
        data,
        IPAC_LED_REPORT_SIZE,
        5000
    );
    
    if (result >= 0) success_count++;
    
    /* Set blue channel */
    memset(data, 0, sizeof(data));
    data[0] = 0x03;
    data[1] = 0x01;
    data[2] = pins.b_pin;
    data[3] = color.b;
    
    result = libusb_control_transfer(
        (libusb_device_handle *)(intptr_t)handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09,
        0x0300,
        0,
        data,
        IPAC_LED_REPORT_SIZE,
        5000
    );
    
    if (result >= 0) success_count++;
    
    return (success_count > 0) ? 0 : -1;
}

/* Set all LEDs based on button configuration array */
int ipac_set_all_leds(int handle, ButtonConfig *buttons, int count, PinMapping *pin_mappings) {
    int success = 0;
    int failed = 0;
    
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
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        libusb_exit(NULL);
        printf("Disconnected from i-pac controller\n");
    }
}
