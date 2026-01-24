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

# retropac auto-kills any running animation daemon
/usr/local/bin/retropac "$1" "$3"
```

Make it executable:

```bash
sudo chmod +x /opt/retropie/configs/all/runcommand-onstart.sh
```

### 4. Set Up LED Animations for EmulationStation Menu

Create the runcommand-onend.sh script:

```bash
sudo nano /opt/retropie/configs/all/runcommand-onend.sh
```

Add the following content:

```bash
#!/bin/bash
# RetroPie runcommand-onend.sh
# This script runs after a game exits, returning to EmulationStation
# Start attract mode animation as a background daemon

/usr/local/bin/retropac --animate rainbow --daemon default default default
```

Make it executable:

```bash
sudo chmod +x /opt/retropie/configs/all/runcommand-onend.sh
```

### 5. Optional: Start Animation on Boot

To start the animation when EmulationStation first loads (before any game is played), add to `/opt/retropie/configs/all/autostart.sh`:

```bash
# Start LED animation before EmulationStation
/usr/local/bin/retropac --animate rainbow --daemon default default default
emulationstation
```

## Animation Options

RetroPac supports LED animations for attract mode:

| Option | Description |
|--------|-------------|
| `--animate <type>` | Animation type: rainbow, breathing, chase, sparkle, color_cycle |
| `--speed <ms>` | Frame delay in milliseconds (default: 50) |
| `--color <hex>` | Base color for breathing/chase (e.g., #FF0000) |
| `--daemon` | Run as background process |

Example animation commands:
```bash
# Rainbow animation (default)
retropac --animate rainbow --daemon default default default

# Slow breathing red animation
retropac --animate breathing --color '#FF0000' --speed 80 --daemon default default default

# Fast chase effect with green
retropac --animate chase --speed 30 --color '#00FF00' --daemon default default default
```

### Self-Managing Daemon

RetroPac automatically manages its daemon:
- Each invocation kills any existing retropac daemon via PID file
- No manual `pkill` required in scripts
- PID stored in `/tmp/retropac.pid`

## Configuration File Structure

### Basic Structure

```json
{
  "ipac_controllers": [ ... ],
  "animation": { ... },
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

### Animation Configuration

Optional animation settings for attract mode:

```json
"animation": {
  "type": "rainbow",
  "speed": 50,
  "color": "#FF0000",
  "colors": ["#FF0000", "#00FF00", "#0000FF"]
}
```

### Top-Level Default Configuration

The top-level `"default"` configuration defines which buttons should be lit when returning to EmulationStation (i.e., when no game is running). This is typically used in the `runcommand-onend.sh` script.

Example:
```json
"default": {
  "P1_COIN": "#FFFF00",
  "P1_START": "#00FF00",
  "P2_COIN": "#FFFF00",
  "P2_START": "#00FF00"
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
  "P1_COIN": "#FFFF00",
  "P1_START": "#FF0000",
  "P1_BUTTON1": "#00FF00",
  "P1_BUTTON2": "#00FF00",
  "P1_BUTTON3": "#00FF00",
  "P1_BUTTON4": "#00FF00",
  "P1_BUTTON5": "#00FF00",
  "P1_BUTTON6": "#00FF00"
}
```

### Pac-Man (MAME)

```json
"pacman": {
  "P1_COIN": "#FFFF00",
  "P1_START": "#FF0000",
  "P1_JOYSTICK": "#0000FF"
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

## Animation Editor Web Interface

RetroPac includes a web-based animation editor that allows you to create, edit, and manage LED animations through a browser interface.

### Starting the Animation Server

```bash
# Start on port 8080 (default)
anim-server /path/to/animations

# Start on a custom port
anim-server /path/to/animations 3000
```

### "Set as Attract" Feature

The animation editor includes a "Set as Attract" button that updates your RetroPie shell scripts to use the selected animation as the attract mode animation (played when in EmulationStation menu).

#### Requirements

For this feature to work, the following prerequisites must be met:

1. **Shell scripts must already exist** with a retropac command line:
   - `/opt/retropie/configs/all/autostart.sh`
   - `/opt/retropie/configs/all/runcommand-onend.sh`
   
   Each script must contain a line with `/usr/local/bin/retropac` or `retropac --`. The "Set as Attract" feature will update these existing lines - it will NOT create new lines.

2. **File ownership**: The shell scripts must be writable by the user running `anim-server`. The easiest way is to change ownership to the `pi` user:

   ```bash
   sudo chown pi:pi /opt/retropie/configs/all/autostart.sh
   sudo chown pi:pi /opt/retropie/configs/all/runcommand-onend.sh
   ```

3. **Example script content** (must be set up before using "Set as Attract"):

   **autostart.sh:**
   ```bash
   /usr/local/bin/retropac --custom rainbow_wave --daemon default default default &
   emulationstation
   ```

   **runcommand-onend.sh:**
   ```bash
   #!/bin/bash
   /usr/local/bin/retropac --custom rainbow_wave --daemon default default default
   ```

When you click "Set as Attract" in the editor, it will:
1. Update both shell scripts to use the selected animation
2. Immediately start playing the animation so you see it right away

## References

- RetroPie Documentation: https://retropie.org.uk/docs/
- Runcommand Documentation: https://github.com/RetroPie/RetroPie-Setup/wiki/runcommand
