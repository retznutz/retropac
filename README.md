# RetroPac - Ultimarc i-pac LED Controller for RetroPie

This program controls LED lighting on Ultimarc i-pac controllers based on the currently running emulator and ROM in RetroPie.

## Features

- Automatically lights up arcade buttons based on game controls
- **LED animations** for attract mode (rainbow, breathing, chase, sparkle, color cycle)
- **Custom animations** - Create your own animation sequences in JSON
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

| Option | Description |
|--------|-------------|
| `--animate <type>` | Run built-in animation (see Animation Types below) |
| `--custom <name>` | Run custom animation by filename (without .json) |
| `--speed <ms>` | Animation speed in milliseconds (default: 50) |
| `--color <hex>` | Base color for animations (e.g., `#FF0000`) |
| `--daemon` | Run as background daemon |
| `--help` | Show help message |

### Animation Types (Built-in)

| Type | Description |
|------|-------------|
| `rainbow` | Rotating rainbow colors across all buttons |
| `breathing` | Smooth fade in/out pulse effect |
| `chase` | Running light with trailing fade |
| `sparkle` | Random sparkle effect |
| `color_cycle` | Cycle through a list of colors |

### Examples

#### Basic Usage

Run with a specific game (sets LEDs based on emulator/ROM config):
```bash
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Use default button configuration (EmulationStation menu):
```bash
retropac default default default
```

#### Built-in Animations

Run rainbow animation as daemon (background):
```bash
retropac --animate rainbow --daemon default default default
```

Run breathing animation with red color at 30ms speed:
```bash
retropac --animate breathing --color '#FF0000' --speed 30 default default default
```

Run chase animation in foreground (Ctrl+C to stop):
```bash
retropac --animate chase --color '#00FF00' default default default
```

Run sparkle animation with blue:
```bash
retropac --animate sparkle --color '#0000FF' --daemon default default default
```

#### Custom Animations

Run a custom animation by filename:
```bash
retropac --custom rainbow_wave default default default
```

Run custom animation as daemon:
```bash
retropac --custom rainbow_wave --daemon default default default
```

Run the idle animation configured in config.json:
```bash
retropac --custom idle --daemon default default default
```

#### RetroPie Integration Examples

When a game starts (in `runcommand-onstart.sh`):
```bash
#!/bin/bash
# Set static LEDs for the game (auto-kills any running animation)
/usr/local/bin/retropac "$1" "$3"
```

When a game ends (in `runcommand-onend.sh`):
```bash
#!/bin/bash
# Start attract mode with custom animation
/usr/local/bin/retropac --custom idle --daemon default default default

# Or use a built-in animation
# /usr/local/bin/retropac --animate rainbow --daemon default default default
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

## Custom Animations

RetroPac supports custom animations defined in JSON files. See [docs/ANIMATIONS.md](docs/ANIMATIONS.md) for the complete format specification.

### Quick Setup

1. Create an `animations` directory alongside your config.json
2. Add animation JSON files (e.g., `rainbow_wave.json`)
3. Configure the idle animation in config.json:

```json
{
  "animations_dir": "animations",
  "idle_animation": "rainbow_wave",
  "ipac_controllers": [...],
  "default": {...},
  "emulators": {...}
}
```

### Animation File Format

```json
{
  "name": "Rainbow Wave",
  "speed": 50,
  "loop": true,
  "frames": [
    {
      "button": "P1_BUTTON1",
      "color": "#FF0000",
      "fade": true,
      "fade_speed_ms": 200,
      "delay_ms": 0
    },
    {
      "button": "P1_BUTTON2",
      "color": "#FF7F00",
      "fade": true,
      "fade_speed_ms": 200,
      "delay_ms": 50
    }
  ]
}
```

| Field | Description |
|-------|-------------|
| `name` | Display name for the animation |
| `speed` | Base timing interval in milliseconds |
| `loop` | Whether animation repeats (true/false) |
| `frames` | Array of frame objects |

### Frame Fields

| Field | Description |
|-------|-------------|
| `button` | Button identifier (e.g., "P1_BUTTON1") |
| `color` | Target color in hex format |
| `fade` | Whether to fade to the color |
| `fade_speed_ms` | Fade duration in milliseconds |
| `delay_ms` | Delay before frame executes (0 = immediate) |

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
├── animations/             # Custom animation files
│   ├── rainbow_wave.json
│   ├── pulse_red.json
│   └── startup_sequence.json
├── docs/                   # Documentation
│   └── ANIMATIONS.md       # Custom animation format docs
├── tools/                  # Utility tools
│   └── rgbcmd2retropac.c   # RGBcommander converter
├── config.example.json     # Example configuration
├── Makefile
└── README.md
```

## License

MIT License
