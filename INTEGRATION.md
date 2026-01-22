# INTEGRATION.md - RetroPie Integration Guide

## Overview

This guide explains how to integrate RetroPac with RetroPie to automatically control arcade button LEDs based on the currently running game.

## RetroPie Architecture

RetroPie uses a script-based system to trigger events when games are launched. The key scripts are:

- `runcommand-onstart.sh` - Executed when a game starts
- `runcommand-onend.sh` - Executed when a game ends

## Installation Steps

### 1. Install RetroPac

```bash
cd /path/to/retropac
make
sudo make install
```

This will install the `retropac` binary to `/usr/local/bin/`.

### 2. Create Configuration

Create the configuration directory:

```bash
mkdir -p /home/pi/RetroPie/configs/retropac
```

Copy and customize the example configuration:

```bash
cp config.example.json /home/pi/RetroPie/configs/retropac/config.json
nano /home/pi/RetroPie/configs/retropac/config.json
```

### 3. Set Up runcommand Scripts

Create the runcommand-onstart.sh script:

```bash
sudo nano /opt/retropie/configs/all/runcommand-onstart.sh
```

Add the following content:

```bash
#!/bin/bash
# RetroPie runcommand-onstart.sh
# Parameters:
# $1 = emulator
# $2 = emulator_command_line
# $3 = rom_path
# $4 = rom_name (basename without extension)

/usr/local/bin/retropac "$1" "$3"
```

Make it executable:

```bash
sudo chmod +x /opt/retropie/configs/all/runcommand-onstart.sh
```

### 4. Optional: Turn Off LEDs When Game Ends

Create the runcommand-onend.sh script:

```bash
sudo nano /opt/retropie/configs/all/runcommand-onend.sh
```

Add the following content:

```bash
#!/bin/bash
# RetroPie runcommand-onend.sh
# This script runs after a game exits, returning to EmulationStation
# Set default button colors using the top-level "default" configuration

/usr/local/bin/retropac "default" "default" "default"
```

Make it executable:

```bash
sudo chmod +x /opt/retropie/configs/all/runcommand-onend.sh
```

## Configuration File Structure

### Basic Structure

```json
{
  "ipac_controllers": [ ... ],
  "default": { ... },
  "emulators": {
    "emulator_name": {
      "roms": {
        "rom_name": { ... },
        "default": { ... }
      }
    }
  }
}
```

### Top-Level Default Configuration

The top-level `"default"` configuration defines which buttons should be lit when returning to EmulationStation (i.e., when no game is running). This is typically used in the `runcommand-onend.sh` script.

Example:
```json
"default": {
  "P1_COIN": {"r": 255, "g": 255, "b": 0},
  "P1_START": {"r": 0, "g": 255, "b": 0},
  "P2_COIN": {"r": 255, "g": 255, "b": 0},
  "P2_START": {"r": 0, "g": 255, "b": 0}
}
```

To activate the top-level default configuration, call:
```bash
/usr/local/bin/retropac "default" "default" "default"
```

### Emulator Names

Common RetroPie emulator names:
- `mame` - MAME arcade emulator
- `nes` - Nintendo Entertainment System
- `snes` - Super Nintendo
- `megadrive` - Sega Genesis/Mega Drive
- `n64` - Nintendo 64
- `psx` - PlayStation 1
- `arcade` - FinalBurn Alpha or other arcade emulators

### ROM Names

ROM names are extracted from the filename without extension:
- `/home/pi/RetroPie/roms/mame/sf2.zip` → `sf2`
- `/home/pi/RetroPie/roms/nes/mario.nes` → `mario`

### Default Configuration

Always include a `"default"` ROM configuration for each emulator. This will be used when a specific ROM configuration is not found.

## Example Configurations

### Street Fighter II (MAME)

```json
"sf2": {
  "P1_COIN": {"r": 255, "g": 255, "b": 0},
  "P1_START": {"r": 255, "g": 0, "b": 0},
  "P1_BUTTON1": {"r": 0, "g": 255, "b": 0},
  "P1_BUTTON2": {"r": 0, "g": 255, "b": 0},
  "P1_BUTTON3": {"r": 0, "g": 255, "b": 0},
  "P1_BUTTON4": {"r": 0, "g": 255, "b": 0},
  "P1_BUTTON5": {"r": 0, "g": 255, "b": 0},
  "P1_BUTTON6": {"r": 0, "g": 255, "b": 0}
}
```

### Pac-Man (MAME)

```json
"pacman": {
  "P1_COIN": {"r": 255, "g": 255, "b": 0},
  "P1_START": {"r": 255, "g": 0, "b": 0},
  "P1_JOYSTICK": {"r": 0, "g": 0, "b": 255}
}
```

## Testing

### Test Manually

Before integrating with RetroPie, test the program manually:

```bash
/usr/local/bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

### View Logs

Check if the script is being called:

```bash
# Add logging to runcommand-onstart.sh
echo "$(date): Emulator=$1, ROM=$3" >> /tmp/retropac.log
/usr/local/bin/retropac "$1" "$3" >> /tmp/retropac.log 2>&1
```

View the log:

```bash
tail -f /tmp/retropac.log
```

## Troubleshooting

### Script Not Running
- Verify script exists: `ls -l /opt/retropie/configs/all/runcommand-onstart.sh`
- Check permissions: Should be executable (`-rwxr-xr-x`)
- Check for syntax errors: `bash -n /opt/retropie/configs/all/runcommand-onstart.sh`

### Wrong Emulator Name
- Check what RetroPie is actually passing by adding logging
- RetroPie may use different emulator names than expected

### Configuration Not Found
- Verify the ROM name being passed matches your configuration
- Check that the config file is valid JSON
- Use the default configuration as a fallback

## Advanced Features

### Per-Emulator Defaults

You can have different default configurations for each emulator:

```json
"mame": {
  "roms": {
    "default": { "P1_COIN": ..., "P1_START": ... }
  }
},
"nes": {
  "roms": {
    "default": { "P1_START": ..., "P1_BUTTON1": ... }
  }
}
```

### Multiple Controllers

If you have multiple i-pac controllers, list them all:

```json
"ipac_controllers": [
  {
    "device": "ipac-ultimate-1",
    "vendor_id": "0xd208",
    "product_id": "0x0310"
  },
  {
    "device": "ipac-ultimate-2",
    "vendor_id": "0xd208",
    "product_id": "0x0310"
  }
]
```

Note: Current implementation only uses the first controller. Multi-controller support would require code modifications.

## References

- RetroPie Documentation: https://retropie.org.uk/docs/
- Runcommand Documentation: https://github.com/RetroPie/RetroPie-Setup/wiki/runcommand
