# Quick Start Guide

## Prerequisites

- Raspberry Pi with RetroPie installed
- Ultimarc i-pac controller connected via USB
- SSH access or terminal on the Pi

## Quick Install (Recommended)

Run the installation script:

```bash
./install.sh
```

This handles everything: dependencies, Node.js, building, and installation.

**Options:**
- `--no-web` - Skip web interface build
- `--no-server` - Skip anim-server build
- `--no-install` - Build only, don't install

Then skip to **Step 3: Set Up USB Permissions**.

---

## Manual Installation (5 minutes)

### 1. Install Dependencies

```bash
sudo apt-get update
sudo apt-get install libjson-c-dev libusb-1.0-0-dev build-essential
```

### 2. Build and Install

```bash
cd /path/to/retropac
make
sudo make install
```

### 3. Set Up USB Permissions

```bash
sudo nano /etc/udev/rules.d/99-ipac.rules
```

Add:
```
SUBSYSTEM=="usb", ATTRS{idVendor}=="d209", MODE="0666"
```

Reload:
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### 4. Configure RetroPie Integration

Create `/opt/retropie/configs/all/runcommand-onstart.sh`:
```bash
#!/bin/bash
/usr/local/bin/retropac "$1" "$3"
```

Create `/opt/retropie/configs/all/runcommand-onend.sh`:
```bash
#!/bin/bash
/usr/local/bin/retropac --animate rainbow --daemon default default default
```

Make executable:
```bash
sudo chmod +x /opt/retropie/configs/all/runcommand-*.sh
```

### 5. Edit Configuration

```bash
sudo nano /etc/retropac/config.json
```

Update `vendor_id` and `product_id` to match your i-pac (use `lsusb | grep -i ultimarc`).

## Test

```bash
# Test static LEDs
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip

# Test animation
retropac --animate rainbow default default default
```

## Done!

Your arcade LEDs will now:
- Show game-specific colors when a game launches
- Run attract animations in EmulationStation menu
