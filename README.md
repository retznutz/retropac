# RetroPac - Ultimarc i-pac LED Controller for RetroPie

This program controls LED lighting on Ultimarc i-pac controllers based on the currently running emulator and ROM in RetroPie.

## Features

- Automatically lights up arcade buttons based on game controls
- **LED animations** for attract mode (rainbow, breathing, chase, sparkle, color cycle)
- Supports multiple Ultimarc i-pac controller models
- JSON-based configuration for emulators, ROMs, and button mappings
- Self-managing daemon mode (auto-kills previous instance)
- Integrates with RetroPie's runcommand scripts

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
# When a game starts, set static LEDs (auto-kills any running animation)
/usr/local/bin/retropac "$1" "$3"
```

Add to `/opt/retropie/configs/all/runcommand-onend.sh`:

```bash
#!/bin/bash
# When returning to EmulationStation, start attract mode animation
/usr/local/bin/retropac --animate rainbow --daemon default default default
```

## Usage

```bash
retropac [options] <emulator> <rom_path> [mode]
```

### Command Line Options

| Option | Long Form | Description |
|--------|-----------|-------------|
| `-a <type>` | `--animate <type>` | Run LED animation (see Animation Types below) |
| `-s <ms>` | `--speed <ms>` | Animation speed in milliseconds (default: 50) |
| `-c <hex>` | `--color <hex>` | Base color for animations (e.g., `#FF0000`) |
| `-d` | `--daemon` | Run as background daemon |
| `-h` | `--help` | Show help message |

### Animation Types

| Type | Description |
|------|-------------|
| `rainbow` | Rotating rainbow colors across all buttons |
| `breathing` | Smooth fade in/out pulse effect |
| `chase` | Running light with trailing fade |
| `sparkle` | Random sparkle effect |
| `color_cycle` | Cycle through a list of colors |

### Examples

Run with a specific game:
```bash
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Use default button configuration (EmulationStation menu):
```bash
retropac default default default
```

Run rainbow animation as daemon:
```bash
retropac --animate rainbow --daemon default default default
```

Run breathing animation with red color at 30ms speed:
```bash
retropac -a breathing -c '#FF0000' -s 30 default default default
```

Run chase animation (foreground, Ctrl+C to stop):
```bash
retropac --animate chase --color '#00FF00' default default default
```

### Self-Managing Daemon

RetroPac automatically manages its daemon process:
- Each invocation kills any existing retropac daemon
- PID is stored in `/tmp/retropac.pid`
- No need for manual `pkill` in shell scripts

## Supported Buttons

- P1_COIN, P2_COIN, P3_COIN, P4_COIN
- P1_START, P2_START, P3_START, P4_START
- P1_BUTTON1-6, P2_BUTTON1-6, P3_BUTTON1-6, P4_BUTTON1-6
- P1_JOYSTICK, P2_JOYSTICK, P3_JOYSTICK, P4_JOYSTICK
- P1_TRACKBALL, P2_TRACKBALL, P3_TRACKBALL, P4_TRACKBALL

## Animation Configuration (Optional)

You can add default animation settings to your `config.json`:

```json
{
  "animation": {
    "type": "rainbow",
    "speed": 50,
    "color": "#FF0000",
    "colors": ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF"]
  },
  "ipac_controllers": [...],
  "default": {...},
  "emulators": {...}
}
```

| Field | Description |
|-------|-------------|
| `type` | Default animation type |
| `speed` | Frame delay in milliseconds |
| `color` | Base color for breathing/chase effects |
| `colors` | Color array for color_cycle animation |

## Project Structure

```
retropac/
├── src/                    # Source files
│   ├── main.c              # Main program entry
│   ├── config.c            # JSON configuration parsing
│   ├── ipac.c              # i-pac USB communication
│   └── animation.c         # LED animation engine
├── include/                # Header files
│   └── retropac.h
├── docs/                   # Documentation
├── tools/                  # Utility tools
│   └── rgbcmd2retropac.c   # RGBcommander converter
├── config.example.json     # Example configuration
├── Makefile
└── README.md
```

## License

MIT License
