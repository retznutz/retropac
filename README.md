# RetroPac - Ultimarc i-pac LED Controller for RetroPie

This program controls LED lighting on Ultimarc i-pac controllers based on the currently running emulator and ROM in RetroPie.

## Features

- Automatically lights up arcade buttons based on game controls
- Supports multiple Ultimarc i-pac controller models
- JSON-based configuration for emulators, ROMs, and button mappings
- Integrates with RetroPie's runcommand-onstart.sh

## Requirements

- Raspberry Pi (tested on Raspberry Pi 3/4)
- RetroPie
- Ultimarc i-pac controller (connected via USB)
- libjson-c library for JSON parsing
- libusb-1.0 for USB communication
- libxml2 (optional, for RGBcommander config converter)

## Installation

### Install Dependencies

```bash
sudo apt-get update
sudo apt-get install libjson-c-dev libusb-1.0-0-dev build-essential

# Optional: for RGBcommander config converter
sudo apt-get install libxml2-dev
```

### Build

```bash
make
```

### Build RGBcommander Converter (Optional)

If you have an existing RGBcommander configuration (`rgbcmdd.xml`), you can convert it to RetroPac format:

```bash
make converter
./rgbcmd2retropac rgbcmdd.xml config.json
```

### Install

```bash
sudo make install
```

This will install the binary to `/usr/local/bin/retropac`.

## Configuration

### JSON Config File

Create a configuration file at `/home/pi/RetroPie/configs/retropac/config.json`:

```json
{
  "ipac_controllers": [
    {
      "device": "ipac-ultimate",
      "vendor_id": "0xd209",
      "product_id": "0x0410"
    }
  ],
  "default": {
    "P1_COIN": "#FFFF00",
    "P1_START": "#FF0000",
    "P2_COIN": "#FFFF00",
    "P2_START": "#FF0000"
  },
  "emulators": {
    "mame": {
      "roms": {
        "sf2": {
          "P1_COIN": "#FFFF00",
          "P1_START": "#FF0000",
          "P1_BUTTON1": "#00FF00",
          "P1_BUTTON2": "#00FF00",
          "P1_BUTTON3": "#00FF00",
          "P1_BUTTON4": "#00FF00",
          "P1_BUTTON5": "#00FF00",
          "P1_BUTTON6": "#00FF00",
          "P1_JOYSTICK": "#FFFFFF"
        },
        "default": {
          "P1_COIN": "#FFFF00",
          "P1_START": "#FF0000",
          "P2_COIN": "#FFFF00",
          "P2_START": "#FF0000"
        }
      }
    }
  }
}
```

### RetroPie Integration

Add to `/opt/retropie/configs/all/runcommand-onstart.sh`:

```bash
#!/bin/bash
/usr/local/bin/retropac "$1" "$3"
```

Add to `/opt/retropie/configs/all/runcommand-onend.sh`:

```bash
#!/bin/bash
/usr/local/bin/retropac "default" "default" "default"
```

## Usage

```bash
retropac <emulator> <rom_path> [mode]
```

Example:
```bash
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

To use the default button configuration (e.g., when returning to EmulationStation):
```bash
retropac default default default
```

## Supported Buttons

- P1_COIN, P2_COIN, P3_COIN, P4_COIN
- P1_START, P2_START, P3_START, P4_START
- P1_BUTTON1-6, P2_BUTTON1-6, P3_BUTTON1-6, P4_BUTTON1-6
- P1_JOYSTICK, P2_JOYSTICK, P3_JOYSTICK, P4_JOYSTICK
- P1_TRACKBALL, P2_TRACKBALL, P3_TRACKBALL, P4_TRACKBALL

## License

MIT License
