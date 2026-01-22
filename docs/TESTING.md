# TESTING.md - Testing Guide

## Prerequisites

Before testing, ensure all dependencies are installed:

```bash
sudo apt-get update
sudo apt-get install libjson-c-dev libusb-1.0-0-dev build-essential
```

## Build Tests

### 1. Clean Build

```bash
make clean
make
```

Expected output:
- No compilation errors
- `bin/retropac` executable created

### 2. Syntax Check

```bash
# Check for syntax errors without building
gcc -fsyntax-only -Wall -Wextra -std=c11 -Iinclude src/*.c
```

## Unit Tests

### Test 1: Button Name Conversion

Create a simple test program:

```c
#include "retropac.h"
#include <stdio.h>

int main() {
    // Test button name to enum
    ButtonType btn = button_name_to_enum("P1_BUTTON1");
    printf("P1_BUTTON1 = %d\n", btn);
    
    // Test enum to button name
    const char *name = button_enum_to_name(P1_COIN);
    printf("P1_COIN = %s\n", name);
    
    return 0;
}
```

### Test 2: ROM Name Extraction

Test with various paths:

```bash
./bin/retropac test /home/pi/RetroPie/roms/mame/sf2.zip
./bin/retropac test /home/pi/RetroPie/roms/nes/mario.nes
./bin/retropac test /path/to/game.bin
```

Expected: ROM name extracted correctly (sf2, mario, game)

### Test 3: JSON Configuration Loading

Test with example configuration:

```bash
cp config.example.json /tmp/test-config.json
# Run program with test config
./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected output:
- Configuration loads successfully
- ROM configuration found
- Button mappings displayed

## Integration Tests

### Test 1: RetroPie Script Simulation

```bash
# Simulate what RetroPie would call
/usr/local/bin/retropac "mame" "/home/pi/RetroPie/roms/mame/sf2.zip"
/usr/local/bin/retropac "nes" "/home/pi/RetroPie/roms/nes/mario.nes"
/usr/local/bin/retropac "snes" "/home/pi/RetroPie/roms/snes/zelda.smc"
```

### Test 2: Default Configuration Fallback

```bash
# Test with ROM not in config (should use default)
./bin/retropac mame /home/pi/RetroPie/roms/mame/unknown-game.zip
```

Expected: Uses "default" configuration for MAME

### Test 3: Missing Emulator

```bash
# Test with emulator not in config
./bin/retropac dreamcast /path/to/game.gdi
```

Expected: Error message about missing emulator configuration

## Hardware Tests

### Test 1: Device Detection

```bash
# Check if i-pac is detected
lsusb | grep "d208"
```

Expected: Ultimarc device listed

### Test 2: USB Permissions

```bash
# Check USB device permissions
ls -l /dev/bus/usb/$(lsusb | grep "d208" | awk '{print $2,$4}' | tr ' ' '/')
```

Expected: Device readable by current user

### Test 3: LED Control (with hardware)

```bash
# Run with actual hardware connected
sudo ./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected:
- i-pac controller detected
- LED colors set successfully
- Physical LEDs light up with correct colors

### Test 4: Simulation Mode (without hardware)

```bash
# Run without hardware (will run in simulation mode)
./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected:
- Program runs without errors
- Shows what LEDs would be set
- No USB errors (runs in simulation mode)

## Stress Tests

### Test 1: Rapid Switching

```bash
# Quickly switch between games
for game in sf2 pacman galaga; do
    ./bin/retropac mame /home/pi/RetroPie/roms/mame/$game.zip
    sleep 1
done
```

### Test 2: Large Configuration

Create a config with many ROMs and test loading time:

```bash
time ./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected: Loads in reasonable time (< 1 second)

## Error Handling Tests

### Test 1: Invalid Arguments

```bash
./bin/retropac
./bin/retropac mame
./bin/retropac "" ""
```

Expected: Usage message displayed

### Test 2: Invalid Config File

```bash
# Test with missing config
./bin/retropac mame /path/to/rom.zip
```

Expected: Error message about missing config file

### Test 3: Malformed JSON

Create a config with syntax errors and test:

```bash
echo '{invalid json}' > /tmp/bad-config.json
# Edit program to use /tmp/bad-config.json
./bin/retropac mame /path/to/rom.zip
```

Expected: JSON parsing error message

### Test 4: Invalid Color Values

Test with colors outside valid range (should handle gracefully)

## Memory Tests

### Test 1: Memory Leaks

```bash
# Install valgrind if not present
sudo apt-get install valgrind

# Run with valgrind
valgrind --leak-check=full ./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected: No memory leaks reported

### Test 2: Multiple Runs

```bash
# Run multiple times to check for memory issues
for i in {1..100}; do
    ./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip > /dev/null
done
```

Expected: No crashes, consistent behavior

## Performance Tests

### Test 1: Startup Time

```bash
time ./bin/retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
```

Expected: Completes quickly (< 1 second preferred)

### Test 2: Config Loading Time

Test with large configuration file (many emulators and ROMs)

## Animation Tests

### Test 1: Animation Types

```bash
# Test each animation type
./bin/retropac --animate rainbow default default default &
sleep 5 && kill $!

./bin/retropac --animate breathing --color '#FF0000' default default default &
sleep 5 && kill $!

./bin/retropac --animate chase --color '#00FF00' default default default &
sleep 5 && kill $!

./bin/retropac --animate sparkle default default default &
sleep 5 && kill $!
```

### Test 2: Daemon Mode

```bash
# Start as daemon
./bin/retropac --animate rainbow --daemon default default default

# Verify it's running
ps aux | grep retropac
cat /tmp/retropac.pid

# Kill it
./bin/retropac default default default  # This auto-kills the daemon
```

### Test 3: Self-Managing Daemon

```bash
# Start daemon
./bin/retropac --animate rainbow --daemon default default default
OLD_PID=$(cat /tmp/retropac.pid)

# Start new daemon (should kill old one)
./bin/retropac --animate breathing --daemon default default default
NEW_PID=$(cat /tmp/retropac.pid)

# Verify old process is gone
! kill -0 $OLD_PID 2>/dev/null && echo "Old daemon killed successfully"
```

## Troubleshooting Common Issues

### Issue: Compilation Errors

**Solution**: Ensure all dependencies are installed:
```bash
sudo apt-get install libjson-c-dev libusb-1.0-0-dev
```

### Issue: USB Device Not Found

**Solutions**:
1. Check physical connection
2. Verify device with `lsusb`
3. Check udev rules (see HARDWARE.md)
4. Try with `sudo`

### Issue: LEDs Not Lighting Up

**Solutions**:
1. Check hardware connections
2. Verify power to LEDs
3. Test with known working configuration
4. Check button-to-pin mapping in code

### Issue: RetroPie Integration Not Working

**Solutions**:
1. Check script permissions
2. Add logging to runcommand-onstart.sh
3. Test manually with same parameters
4. Check RetroPie logs

## Continuous Testing

For development, create a test script:

```bash
#!/bin/bash
# test.sh - Quick test script

echo "Building..."
make clean && make || exit 1

echo "Testing button conversion..."
# Add your test cases here

echo "Testing with example config..."
./retropac mame /home/pi/RetroPie/roms/mame/sf2.zip

echo "All tests passed!"
```

Make it executable and run:

```bash
chmod +x test.sh
./test.sh
```
