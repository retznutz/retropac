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
 */

#define IPAC_LED_REPORT_SIZE 64

/* i-pac button to pin mapping (approximate, may need adjustment based on hardware) */
static int button_to_pin(ButtonType button) {
    /* This is a simplified mapping. In production, this should be 
     * loaded from configuration or based on actual i-pac pinout */
    switch (button) {
        case P1_COIN: return 1;
        case P2_COIN: return 2;
        case P3_COIN: return 3;
        case P4_COIN: return 4;
        
        case P1_START: return 5;
        case P2_START: return 6;
        case P3_START: return 7;
        case P4_START: return 8;
        
        case P1_BUTTON1: return 9;
        case P1_BUTTON2: return 10;
        case P1_BUTTON3: return 11;
        case P1_BUTTON4: return 12;
        case P1_BUTTON5: return 13;
        case P1_BUTTON6: return 14;
        
        case P2_BUTTON1: return 15;
        case P2_BUTTON2: return 16;
        case P2_BUTTON3: return 17;
        case P2_BUTTON4: return 18;
        case P2_BUTTON5: return 19;
        case P2_BUTTON6: return 20;
        
        case P3_BUTTON1: return 21;
        case P3_BUTTON2: return 22;
        case P3_BUTTON3: return 23;
        case P3_BUTTON4: return 24;
        case P3_BUTTON5: return 25;
        case P3_BUTTON6: return 26;
        
        case P4_BUTTON1: return 27;
        case P4_BUTTON2: return 28;
        case P4_BUTTON3: return 29;
        case P4_BUTTON4: return 30;
        case P4_BUTTON5: return 31;
        case P4_BUTTON6: return 32;
        
        case P1_JOYSTICK: return 33;
        case P2_JOYSTICK: return 34;
        case P3_JOYSTICK: return 35;
        case P4_JOYSTICK: return 36;
        
        case P1_TRACKBALL: return 37;
        case P2_TRACKBALL: return 38;
        case P3_TRACKBALL: return 39;
        case P4_TRACKBALL: return 40;
        
        default: return -1;
    }
}

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
int ipac_set_led(int handle, ButtonType button, RGBColor color) {
    unsigned char data[IPAC_LED_REPORT_SIZE];
    int pin = button_to_pin(button);
    int result;
    
    if (pin < 0) {
        fprintf(stderr, "Error: Invalid button type\n");
        return -1;
    }
    
    memset(data, 0, sizeof(data));
    
    /* 
     * LED control packet format (simplified):
     * This is a basic implementation. The actual protocol depends on
     * the i-pac model and firmware version.
     * 
     * Byte 0: Report ID (0x03 for LED control on some models)
     * Byte 1: Command (0x01 for set LED)
     * Byte 2: Pin number
     * Byte 3: Red value
     * Byte 4: Green value
     * Byte 5: Blue value
     */
    data[0] = 0x03; /* Report ID */
    data[1] = 0x01; /* Set LED command */
    data[2] = pin;
    data[3] = color.r;
    data[4] = color.g;
    data[5] = color.b;
    
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
    
    if (result < 0) {
        fprintf(stderr, "Error: LED control transfer failed: %s\n", libusb_error_name(result));
        return -1;
    }
    
    return 0;
}

/* Set all LEDs based on button configuration array */
int ipac_set_all_leds(int handle, ButtonConfig *buttons, int count) {
    int success = 0;
    int failed = 0;
    
    printf("Setting %d button LEDs...\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("  %s -> RGB(%d, %d, %d)\n",
               button_enum_to_name(buttons[i].button),
               buttons[i].color.r,
               buttons[i].color.g,
               buttons[i].color.b);
        
        if (ipac_set_led(handle, buttons[i].button, buttons[i].color) == 0) {
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
