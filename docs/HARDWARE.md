# HARDWARE.md - Ultimarc PAC Hardware Information

## Supported Controllers

This program is designed to work with Ultimarc PAC controllers (I-PAC Ultimate and PacLED64), specifically:

### PAC Ultimate I/O
- **Vendor ID**: 0xd208
- **Product ID**: 0x0310
- **Features**: Full RGB LED support
- **Documentation**: https://www.ultimarc.com/control-interfaces/PACs/PAC-ultimate-io/

### Other PAC Models
The program can be adapted to support other PAC models:
- PAC2 (0xd208:0x0420)
- PAC4 (0xd208:0x0421)
- MinPAC (0xd208:0x0401)

### Find Vendor and Product ID
If you cannot connect to your PAC run

```bash
lsusb | grep -i ultimarc
```

You will see the correct vendor and product ID. Something like :: Bus 001 Device 004: ID d209:0410 Ultimarc PAC Ultimate I/O ::

d209 is the vendorId and 0410 is the productId

## USB Communication

The PAC controllers communicate via USB HID (Human Interface Device) protocol.

### USB Permissions

To allow non-root users to access the PAC controller, create a udev rule:

```bash
sudo nano /etc/udev/rules.d/99-ipac.rules
```

Add the following content:

```
# Ultimarc PAC controllers
SUBSYSTEM=="usb", ATTRS{idVendor}=="d209", MODE="0666"
SUBSYSTEM=="usb_device", ATTRS{idVendor}=="d209", MODE="0666"
```
Make sure your vendorId (d209) matches your config.json vendorId and that matches the vendorId for your PAC.

Reload udev rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## LED Control Protocol

The PAC Ultimate I/O supports RGB LED control via USB HID Feature Reports.

### Actual Protocol Implementation

RetroPac uses the following protocol to control individual LED channels:

```c
// Message format (5 bytes)
data[0] = 0xDD;      // Set LED intensity command
data[1] = led_index; // LED index (0-95)
data[2] = intensity; // Intensity (0-255)
data[3] = 0x00;      // Padding
data[4] = 0x00;      // Padding
```

Each RGB button requires **3 separate commands** - one for each color channel (R, G, B).

For example, setting P1_BUTTON1 to orange (#FF8000):
- Command 1: Pin 25 (red), intensity 255
- Command 2: Pin 26 (green), intensity 128
- Command 3: Pin 27 (blue), intensity 0

### Pin Mapping

The button-to-pin mapping is hardware-specific and depends on how your arcade cabinet is wired. The default mapping in the code is a reference implementation and should be customized based on your setup.

## Hardware Setup

1. **Connect PAC to Raspberry Pi**: Use a USB cable to connect the PAC controller to a USB port on the Raspberry Pi.

2. **Wire Arcade Buttons**: Connect your arcade buttons with LEDs to the PAC controller according to the PAC pinout diagram.

3. **Power Considerations**: The PAC can power LEDs, but for many LEDs, you may need external power. Consult the PAC documentation for power requirements.

4. **Verify Connection**:
   ```bash
   lsusb | grep "d208"
   ```
   You should see the PAC device listed.

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
- The protocol implementation may need adjustment for your specific PAC model/firmware
- Enable debug output in the code to see USB transfer results

## References

- Ultimarc Website: https://www.ultimarc.com/
- PAC Ultimate I/O: https://www.ultimarc.com/control-interfaces/PACs/PAC-ultimate-io/
- USB HID Specification: https://www.usb.org/hid
- libusb Documentation: https://libusb.info/

## Note on Protocol

The LED control protocol implementation in this program is based on general USB HID principles and may need to be adjusted based on:
- Actual PAC firmware version
- Official Ultimarc documentation (if available)
- Testing with real hardware

For production use, it's recommended to:
1. Test with actual hardware
2. Consult Ultimarc support or documentation
3. Use tools like Wireshark or usbmon to analyze the actual USB traffic
4. Adjust the protocol implementation as needed
