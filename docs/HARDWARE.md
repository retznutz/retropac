# HARDWARE.md - Ultimarc i-pac Hardware Information

## Supported Controllers

This program is designed to work with Ultimarc i-pac controllers, specifically:

### I-PAC Ultimate I/O
- **Vendor ID**: 0xd208
- **Product ID**: 0x0310
- **Features**: Full RGB LED support
- **Documentation**: https://www.ultimarc.com/control-interfaces/i-pacs/i-pac-ultimate-io/

### Other I-PAC Models
The program can be adapted to support other i-pac models:
- I-PAC2 (0xd208:0x0420)
- I-PAC4 (0xd208:0x0421)
- Mini-PAC (0xd208:0x0401)

### Find Vendor and Product ID
If you cannot connect to your i-pac run

```bash
lsusb | grep -i ultimarc
```

You will see the correct vendor and product ID. Something like :: Bus 001 Device 004: ID d209:0410 Ultimarc I-PAC Ultimate I/O ::

d209 is the vendorId and 0410 is the productId

## USB Communication

The i-pac controllers communicate via USB HID (Human Interface Device) protocol.

### USB Permissions

To allow non-root users to access the i-pac controller, create a udev rule:

```bash
sudo nano /etc/udev/rules.d/99-ipac.rules
```

Add the following content:

```
# Ultimarc i-pac controllers
SUBSYSTEM=="usb", ATTRS{idVendor}=="d208", MODE="0666"
SUBSYSTEM=="usb_device", ATTRS{idVendor}=="d208", MODE="0666"
```

Reload udev rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## LED Control Protocol

The i-pac Ultimate I/O supports RGB LED control via USB HID Feature Reports.

### Basic Protocol (Simplified)

The actual protocol may vary based on firmware version. This is a basic implementation:

```c
// Feature Report structure (64 bytes)
data[0] = 0x03;      // Report ID
data[1] = 0x01;      // Command: Set LED
data[2] = pin;       // Pin number (1-48)
data[3] = red;       // Red value (0-255)
data[4] = green;     // Green value (0-255)
data[5] = blue;      // Blue value (0-255)
```

### Pin Mapping

The button-to-pin mapping is hardware-specific and depends on how your arcade cabinet is wired. The default mapping in the code is a reference implementation and should be customized based on your setup.

## Hardware Setup

1. **Connect i-pac to Raspberry Pi**: Use a USB cable to connect the i-pac controller to a USB port on the Raspberry Pi.

2. **Wire Arcade Buttons**: Connect your arcade buttons with LEDs to the i-pac controller according to the i-pac pinout diagram.

3. **Power Considerations**: The i-pac can power LEDs, but for many LEDs, you may need external power. Consult the i-pac documentation for power requirements.

4. **Verify Connection**:
   ```bash
   lsusb | grep "d208"
   ```
   You should see the i-pac device listed.

## Troubleshooting

### Device Not Found
- Check USB connection
- Run `lsusb` to verify the device is detected
- Verify vendor/product IDs match your device
- Check udev rules are in place

### Permission Denied
- Ensure udev rules are correctly set up
- Try running with `sudo` as a test
- Check USB device permissions: `ls -l /dev/bus/usb/`

### LEDs Not Responding
- Verify hardware connections
- Check that LEDs are getting power
- The protocol implementation may need adjustment for your specific i-pac model/firmware
- Enable debug output in the code to see USB transfer results

## References

- Ultimarc Website: https://www.ultimarc.com/
- I-PAC Ultimate I/O: https://www.ultimarc.com/control-interfaces/i-pacs/i-pac-ultimate-io/
- USB HID Specification: https://www.usb.org/hid
- libusb Documentation: https://libusb.info/

## Note on Protocol

The LED control protocol implementation in this program is based on general USB HID principles and may need to be adjusted based on:
- Actual i-pac firmware version
- Official Ultimarc documentation (if available)
- Testing with real hardware

For production use, it's recommended to:
1. Test with actual hardware
2. Consult Ultimarc support or documentation
3. Use tools like Wireshark or usbmon to analyze the actual USB traffic
4. Adjust the protocol implementation as needed
