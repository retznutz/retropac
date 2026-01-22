# PROJECT_SUMMARY.md

## RetroPac - Ultimarc i-pac LED Controller for RetroPie

### Project Overview

RetroPac is a C program designed to control LED lighting on Ultimarc i-pac controllers based on the currently running game in RetroPie. When a game starts, the program lights up only the arcade buttons that are used for that specific game, helping users identify which controls are active.

### Key Features

1. **Automatic LED Control**: Lights up arcade button LEDs based on the running game
2. **RetroPie Integration**: Works seamlessly with RetroPie's runcommand system
3. **JSON Configuration**: Flexible, easy-to-edit configuration for games and buttons
4. **Multiple Emulator Support**: Works with MAME, NES, SNES, and other emulators
5. **Default Fallbacks**: Supports default configurations when specific ROM configs aren't found
6. **USB Communication**: Direct USB/HID communication with i-pac controllers
7. **Simulation Mode**: Can run without hardware for testing

### Architecture

#### Components

1. **Main Program (main.c)**
   - Command-line argument parsing
   - ROM name extraction from file path
   - Configuration lookup
   - Workflow orchestration

2. **Configuration Parser (config.c)**
   - JSON parsing using libjson-c
   - Button name to enum conversion
   - Configuration structure management
   - Memory management for config data

3. **i-pac Controller Interface (ipac.c)**
   - USB device initialization using libusb
   - HID communication protocol
   - LED color control
   - Button-to-pin mapping

4. **Header File (retropac.h)**
   - 44 button enumerations (4 players × 11 button types)
   - Data structures for config, ROMs, emulators
   - Function prototypes

#### Data Flow

```
RetroPie RunCommand
    ↓
retropac <emulator> <rom_path>
    ↓
Extract ROM name from path
    ↓
Load JSON configuration
    ↓
Lookup emulator → ROM → buttons
    ↓
Initialize i-pac USB connection
    ↓
Set LED colors for active buttons
```

### Supported Buttons

The program supports 44 different button types across 4 players:

**Coin Buttons**: P1_COIN, P2_COIN, P3_COIN, P4_COIN

**Start Buttons**: P1_START, P2_START, P3_START, P4_START

**Action Buttons**: P[1-4]_BUTTON[1-6] (24 buttons total)

**Controllers**: P1_JOYSTICK, P2_JOYSTICK, P3_JOYSTICK, P4_JOYSTICK

**Trackballs**: P1_TRACKBALL, P2_TRACKBALL, P3_TRACKBALL, P4_TRACKBALL

### Technical Specifications

#### Dependencies
- **libjson-c**: JSON parsing library
- **libusb-1.0**: USB device communication
- **gcc**: C compiler with C11 support

#### Supported Hardware
- Ultimarc i-pac Ultimate I/O (VID: 0xd208, PID: 0x0310)
- Other i-pac models can be supported with configuration adjustments

#### Platform
- Raspberry Pi (all models with USB support)
- Linux-based systems
- RetroPie 4.x+

### File Structure

```
retropac/
├── retropac.h              # Header file with definitions
├── main.c                  # Main program logic
├── config.c                # JSON configuration parser
├── ipac.c                  # i-pac USB communication
├── Makefile                # Build system
├── config.example.json     # Example configuration
├── validate.sh             # Code validation script
├── README.md               # Main documentation
├── HARDWARE.md             # Hardware setup guide
├── INTEGRATION.md          # RetroPie integration guide
├── TESTING.md              # Testing documentation
└── .gitignore              # Git ignore rules
```

### Configuration Format

```json
{
  "ipac_controllers": [
    {
      "device": "ipac-ultimate",
      "vendor_id": "0xd208",
      "product_id": "0x0310"
    }
  ],
  "emulators": {
    "emulator_name": {
      "roms": {
        "rom_name": {
          "BUTTON_NAME": "#RRGGBB"
        }
      }
    }
  }
}
```

Color values can be specified as hex strings with or without the `#` prefix (e.g., `"#FF0000"` or `"FF0000"` for red).

### Installation Steps

1. Install dependencies:
   ```bash
   sudo apt-get install libjson-c-dev libusb-1.0-0-dev build-essential
   ```

2. Build:
   ```bash
   make
   ```

3. Install:
   ```bash
   sudo make install
   ```

4. Configure:
   - Edit `/home/pi/RetroPie/configs/retropac/config.json`
   - Add to `/opt/retropie/configs/all/runcommand-onstart.sh`

### Usage

```bash
retropac <emulator> <rom_path>
```

Examples:
```bash
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
retropac nes /home/pi/RetroPie/roms/nes/mario.nes
retropac snes /home/pi/RetroPie/roms/snes/zelda.smc
```

### Key Design Decisions

1. **JSON Configuration**: Chose JSON for human-readable, easy-to-edit configuration
2. **libjson-c**: Standard, well-maintained JSON library for C
3. **libusb**: Direct USB communication for better control and compatibility
4. **Simulation Mode**: Allows testing without hardware
5. **Default Configs**: Graceful fallback when specific ROM config missing
6. **Button Enums**: Type-safe button identification
7. **Modular Design**: Separate concerns (parsing, USB, main logic)

### Error Handling

- Graceful degradation when i-pac not connected (simulation mode)
- Missing ROM configs fall back to default
- Invalid JSON produces clear error messages
- USB errors don't crash the program
- Missing files produce helpful error messages

### Future Enhancements

Possible improvements for future versions:

1. **Multi-controller Support**: Control multiple i-pac devices simultaneously
2. **Dynamic Pin Mapping**: Load button-to-pin mapping from configuration
3. **Animation Support**: LED animations and patterns
4. **Brightness Control**: Adjustable LED brightness
5. **Web Interface**: Web-based configuration editor
6. **Automatic Discovery**: Auto-detect i-pac controllers
7. **Clear Mode**: Turn off all LEDs when game ends
8. **Profile System**: Quick-switch between different LED schemes

### Testing

The project includes comprehensive testing documentation:

- Unit tests for individual components
- Integration tests with RetroPie
- Hardware tests with actual i-pac devices
- Stress tests for reliability
- Memory leak detection
- Performance benchmarks

### Documentation

Complete documentation provided:

- **README.md**: Quick start and overview
- **HARDWARE.md**: Hardware setup and USB details
- **INTEGRATION.md**: RetroPie integration guide
- **TESTING.md**: Testing procedures
- **PROJECT_SUMMARY.md**: This document

### Code Quality

- Clean, well-commented C code
- Proper memory management (no leaks)
- Error checking on all system calls
- Modular, maintainable structure
- Consistent coding style
- Header guards and proper includes

### License

MIT License - Free to use, modify, and distribute

### Credits

- Developed for RetroPie gaming systems
- Uses Ultimarc i-pac controller hardware
- Built with libjson-c and libusb libraries

---

**Note**: The i-pac LED control protocol is implemented based on general USB HID principles. For production use with actual hardware, the protocol may need fine-tuning based on:
- Specific i-pac model and firmware version
- Official Ultimarc documentation
- Testing with real hardware
- USB packet analysis

The current implementation provides a solid foundation that can be adapted as needed.
