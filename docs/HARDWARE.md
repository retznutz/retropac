# HARDWARE.md - Ultimarc Hardware Information

## Supported Controllers

This program is designed to work with Ultimarc LED controllers. **Multiple controllers can be used simultaneously** - each configured independently in the `ipac_controllers` array.

### Supported Controller Models

| Controller | Vendor ID | Product ID Range | LED Channels | Intensity | Features |
|------------|-----------|------------------|--------------|-----------|----------|
| I-PAC Ultimate I/O | `0xD209` | `0x0410`-`0x0413` | 96 (32 RGB) | 256 levels | Fade |
| PacLED64 | `0xD209` | `0x1401`-`0x1408` | 64 (21 RGB) | 256 levels | Fade, Flash, Scripting |
| NanoLED | `0xD209` | `0x1481`-`0x1484` | 60 (20 RGB) | 256 levels | Fade, Flash, Scripting |
| PacDrive | `0xD209` | `0x1500` | 16 (5 RGB) | On/Off | - |
| U-HID | `0xD209` | `0x1501`-`0x1508` | 16 (5 RGB) | On/Off | - |
| USB Button | `0xD209` | `0x1200` | 3 (1 RGB) | 256 levels | - |

### Device Capabilities

**PWM Intensity Control (256 levels):**
- I-PAC Ultimate I/O, PacLED64, NanoLED, USB Button
- Supports smooth color blending and gradients

**On/Off Only (no PWM):**
- PacDrive, U-HID
- Colors are thresholded at 50% brightness (0-127 = off, 128-255 = on)

**Hardware Features:**
- **Fade**: Hardware-accelerated fade transitions between intensity levels
- **Flash**: Hardware-controlled LED flashing at configurable speeds
- **Scripting**: Record and playback LED sequences stored on the device

### Multiple Controller Setup

RetroPac supports controlling multiple PAC devices simultaneously. This is useful for:
- Large arcade cabinets with more buttons than a single controller can handle
- Multi-player setups with separate controllers per player
- Mixing controller types (e.g., I-PAC Ultimate for buttons + PacLED64 for additional LEDs)

Each controller is configured separately in `config.json`:

```json
{
  "ipac_controllers": [
    {
      "device": "main-controller",
      "vendor_id": "0xd209",
      "product_id": "0x0410",
      "pin_mappings": { ... }
    },
    {
      "device": "secondary-leds",
      "vendor_id": "0xd209",
      "product_id": "0x1401",
      "pin_mappings": { ... }
    }
  ]
}
```

In animations, target specific controllers using the `controller` property (0-based index).

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

## LED Control Protocols

Different Ultimarc devices use different LED control protocols:

### I-PAC Ultimate I/O Protocol

Uses USB HID Control Transfers (Output Reports) on interface 2 (NGC mode) or interface 3 (Game Controller mode):

```c
// Message format (5 bytes)
// wValue = 0x0203 (Report Type: Output, Report ID: 3)
data[0] = 0x03;      // Report ID
data[1] = led_index; // LED index (1-96, 1-based) or command byte
data[2] = intensity; // Intensity (0-255) or parameter
data[3] = 0x00;      // Padding
data[4] = 0x00;      // Padding

// Commands (used in data[1]):
// 0x01-0x60: Individual LED index (1-96, 1-based)
// 0x80:      Set all LEDs to same intensity (data[2] = intensity)
// 0x89:      Random LED states
// 0xC0:      Set fade rate (data[2] = rate, 0=instant, 1-255=slower)
```

### PacLED64 / NanoLED Protocol (64-LED Protocol)

Uses USB HID Interrupt Transfers on interface 0:

```c
// Set single LED (4 bytes)
data[0] = 0x80 | (led_index & 0x3F);  // Command + LED index
data[1] = intensity;                   // Intensity (0-255)
data[2] = 0x00;                        // Padding
data[3] = 0x00;                        // Padding

// Set fade time
data[0] = 0x40;
data[1] = fade_time;  // 0-255 (higher = slower fade)

// Set flash speed for single LED
data[0] = 0xC0 | (led_index & 0x3F);
data[1] = flash_speed;  // 0 = off, 1-255 = speed
```

### PacDrive / U-HID Protocol (16-LED Protocol)

Uses a 16-bit bitmask for on/off state:

```c
// Message format (4 bytes)
data[0] = 0x00;                    // Command
data[1] = (led_states >> 8) & 0xFF; // Upper 8 bits
data[2] = led_states & 0xFF;        // Lower 8 bits
```

### USB Button Protocol

Uses USB HID Control Transfers:

```c
// Set RGB color (5 bytes)
data[0] = 0x03;  // Color command
data[1] = red;   // Red intensity (0-255)
data[2] = green; // Green intensity (0-255)
data[3] = blue;  // Blue intensity (0-255)
```

---

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
