# PROJECT_SUMMARY.md

## RetroPac - Ultimarc PAC LED Controller for RetroPie

### Project Overview

RetroPac is a C program designed to control LED lighting on Ultimarc PAC controllers based on the currently running game in RetroPie. When a game starts, the program lights up only the arcade buttons that are used for that specific game, helping users identify which controls are active.

### Key Features

1. **Automatic LED Control**: Lights up arcade button LEDs based on the running game
2. **LED Animations**: Attract mode animations (rainbow, breathing, chase, sparkle, color cycle)
3. **Custom Animations**: Create frame-by-frame LED animations in JSON format
4. **Web-Based Animation Editor**: Visual editor with timeline, drag-and-drop frame reordering, and live preview
5. **Web-Based Config Editor**: Configure pin mappings, button colors, and button labels from a browser
6. **Button Labels**: Define friendly names for buttons (e.g., "Punch", "Kick") displayed throughout the UI
7. **Single Button Control**: Test individual LEDs with `--set-button` for verifying pin mappings
8. **Self-Managing Daemon**: Auto-kills previous instance via PID file
9. **RetroPie Integration**: Works seamlessly with RetroPie's runcommand system
10. **JSON Configuration**: Flexible, easy-to-edit configuration for games and buttons with hex color format
11. **Multiple Emulator Support**: Works with MAME, NES, SNES, and other emulators
12. **Multiple Controller Support**: Control multiple PAC devices simultaneously with per-controller configuration
12. **Default Configurations**: 
   - Top-level default for EmulationStation menu (when no game is running)
   - Per-emulator defaults when specific ROM configs aren't found
13. **USB Communication**: Direct USB/HID communication with PAC controllers
14. **Simulation Mode**: Can run without hardware for testing

### Architecture

#### Components

1. **Main Program (src/main.c)**
   - Command-line argument parsing (including animation options)
   - ROM name extraction from file path
   - Configuration lookup
   - Daemon management with PID file
   - Workflow orchestration

2. **Configuration Parser (src/config.c)**
   - JSON parsing using libjson-c
   - Button name to enum conversion
   - Animation config parsing
   - Configuration structure management
   - Memory management for config data

3. **PAC Controller Interface (src/ipac.c)**
   - USB device initialization using libusb
   - HID communication protocol
   - LED color control
   - Button-to-pin mapping

4. **Animation Engine (src/animation.c)**
   - Multiple animation types (rainbow, breathing, chase, sparkle, color_cycle)
   - Signal handling for graceful shutdown
   - Configurable speed and colors
   - Frame-based animation loop

5. **Header File (include/retropac.h)**
   - 44 button enumerations (4 players × 11 button types)
   - Animation type definitions
   - Data structures for config, ROMs, emulators
   - Function prototypes

#### Data Flow

```
RetroPie RunCommand
    ↓
retropac [options] <emulator> <rom_path>
    ↓
Kill existing daemon (if any)
    ↓
Extract ROM name from path
    ↓
Load JSON configuration
    ↓
Lookup emulator → ROM → buttons
    ↓
Initialize PAC USB connection
    ↓
Set LED colors for active buttons
    ↓
(If --animate) Run animation loop as daemon
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
- Ultimarc PAC Ultimate I/O (VID: 0xd208, PID: 0x0310)
- Other PAC models can be supported with configuration adjustments

#### Platform
- Raspberry Pi (all models with USB support)
- Linux-based systems
- RetroPie 4.x+

### File Structure

```
retropac/
├── src/                        # Source files
│   ├── main.c                  # Main program logic
│   ├── config.c                # JSON configuration parser
│   ├── ipac.c                  # PAC USB communication
│   └── animation.c             # LED animation engine
├── include/                    # Header files
│   └── retropac.h              # Main header with definitions
├── web/                        # Web-based editor (Nuxt 3)
│   ├── pages/                  # Page components
│   │   ├── index.vue           # Animation editor
│   │   └── config.vue          # Config editor
│   ├── components/             # Vue components
│   ├── composables/            # Vue composables
│   └── assets/                 # CSS assets
├── docs/                       # Documentation
│   ├── HARDWARE.md             # Hardware setup guide
│   ├── INTEGRATION.md          # RetroPie integration guide
│   ├── TESTING.md              # Testing documentation
│   ├── EDITOR_SETUP.md         # Web editor setup guide
│   ├── ANIMATIONS.md           # Custom animation format
│   └── PROJECT_SUMMARY.md      # This document
├── tools/                      # Utility tools
│   ├── anim-server.c           # HTTP server for web editor
│   ├── rgbcmd2retropac.c       # RGBcommander converter
│   └── rgbcmdd.xml             # Example RGBcommander config
├── animations/                 # Animation JSON files
├── obj/                        # Build output (object files)
├── bin/                        # Build output (executables)
├── Makefile                    # Build system
├── install.sh                  # Automated installation script
├── config.example.json         # Example configuration
├── validate.sh                 # Code validation script
├── README.md                   # Main documentation
└── .gitignore                  # Git ignore rules
```

### Configuration Format

```json
{
  "animations_dir": "/home/pi/retropac/animations",
  "ipac_controllers": [
    {
      "device": "ipac-ultimate",
      "vendor_id": "0xd209",
      "product_id": "0x0410",
      "pin_mappings": {
        "P1_BUTTON1": { "r_pin": 1, "g_pin": 2, "b_pin": 3 }
      },
      "default": {
        "P1_BUTTON1": "#FFFF00"
      },
      "button_labels": {
        "P1_BUTTON1": "Punch"
      }
    },
    {
      "device": "pacled64",
      "vendor_id": "0xd209",
      "product_id": "0x1401",
      "pin_mappings": { ... },
      "default": { ... },
      "button_labels": { ... }
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

**Multiple Controllers**: The `ipac_controllers` array supports multiple PAC devices. Each controller has its own:
- `device`: Friendly name for the controller
- `vendor_id` / `product_id`: USB identifiers for this specific controller
- `pin_mappings`: Button-to-pin mappings unique to this controller's wiring
- `default`: Default button colors for this controller
- `button_labels`: Friendly button names for this controller

The optional `"button_labels"` section within each controller defines friendly names for buttons, displayed in the web UI for easier identification.

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
   - Edit `/etc/retropac/config.json`
   - Add to `/opt/retropie/configs/all/runcommand-onstart.sh`

### Usage

```bash
retropac [options] <emulator> <rom_path> [mode]
```

#### Command Line Options

| Option | Description |
|--------|-------------|
| `--config <path>` | Config file path (default: /etc/retropac/config.json) |
| `--animate <type>` | Run LED animation (rainbow, breathing, chase, sparkle, color_cycle) |
| `--custom <name>` | Run custom animation by filename (without .json) |
| `--speed <ms>` | Animation speed in milliseconds (default: 50) |
| `--color <hex>` | Base color for animations (e.g., #FF0000) |
| `--set-button <name> <color>` | Set a single button LED (e.g., --set-button P1_BUTTON1 #FFFFFF) |
| `--daemon` | Run as background daemon |
| `--quiet` | Suppress all console output |
| `--help` | Show help message |

Examples:
```bash
# Game-specific LED control
retropac mame /home/pi/RetroPie/roms/mame/sf2.zip
retropac nes /home/pi/RetroPie/roms/nes/mario.nes
retropac snes /home/pi/RetroPie/roms/snes/zelda.smc

# Default configuration (for EmulationStation menu)
retropac default default default

# Rainbow animation as daemon
retropac --animate rainbow --daemon default default default

# Breathing animation with red color
retropac --animate breathing --color '#FF0000' --speed 30 default default default

# Set a single button LED (useful for testing pin mappings)
retropac --set-button P1_BUTTON1 '#FFFFFF'

# Turn off a single button LED
retropac --set-button P1_BUTTON1 '#000000'
```

### Key Design Decisions

1. **JSON Configuration**: Chose JSON for human-readable, easy-to-edit configuration
2. **Hex Color Format**: Colors specified as hex strings (#RRGGBB) for simplicity and readability
3. **libjson-c**: Standard, well-maintained JSON library for C
4. **libusb**: Direct USB communication for better control and compatibility
5. **Simulation Mode**: Allows testing without hardware
6. **Default Configs**: Multiple levels - top-level default for menu, per-emulator defaults for games
7. **Button Enums**: Type-safe button identification
8. **Modular Design**: Separate concerns (parsing, USB, animation, main logic)
9. **Self-Managing Daemon**: PID file ensures clean process management
10. **Organized Project Structure**: src/, include/, docs/, tools/ directories

### Error Handling

- Graceful degradation when PAC not connected (simulation mode)
- Missing ROM configs fall back to default
- Invalid JSON produces clear error messages
- USB errors don't crash the program
- Missing files produce helpful error messages

### Web-Based Animation Editor

RetroPac includes a full-featured web interface for creating and editing LED animations:

- **Visual arcade panel**: Click buttons to select them for color assignment
- **Timeline editor**: Add, edit, and delete animation frames
- **Drag-and-drop reordering**: Rearrange frames by dragging them in the timeline
- **Live preview**: Play animations with real-time frame highlighting
- **Animation settings**: Configure speed, looping, and fade effects
- **Full-width responsive layout**: Works on any screen size

### Web-Based Config Editor

The web interface also includes a configuration editor:

- **Button mapping**: Configure button types and enable/disable individual buttons
- **ROM colors**: Set default LED colors for each button
- **Color picker with hex input**: Visual selection or precise hex values
- **Live updates**: Changes apply immediately when saved

Access the config editor at: `http://<your-pi-ip>:8080/config`

See [EDITOR_SETUP.md](EDITOR_SETUP.md) for setup instructions.

### Installation Script

RetroPac includes an automated installation script (`install.sh`) that:

- Installs all required dependencies
- Checks for Node.js and installs if needed
- Builds the C binaries
- Builds the web interface
- Installs everything to system locations

```bash
# Quick install
curl -sSL https://raw.githubusercontent.com/your-repo/retropac/main/install.sh | bash
```

Options: `--no-web`, `--no-server`, `--no-install`

### Animation Smoothness

RetroPac uses several techniques for smooth LED animations:

- **Gamma correction**: 2.2 gamma lookup table for natural brightness perception
- **Easing functions**: Smooth transitions using cubic/quintic interpolation
  - `smooth_step()`: Cubic Hermite interpolation for breathing
  - `ease_out()`: Quadratic ease-out for chase tail
- **Sub-frame interpolation**: Custom animations use smooth_step for color lerping

### Future Enhancements

Possible improvements for future versions:

1. ~~**Multi-controller Support**: Control multiple PAC devices simultaneously~~ ✓ Implemented
2. **Animation from Config**: Load animation settings from config.json automatically
3. **Per-Button Animations**: Different animations for different button groups
4. **Automatic Discovery**: Auto-detect PAC controllers
5. **Sound Reactive**: LEDs respond to audio input
6. **Profile System**: Quick-switch between different LED schemes

### Testing

The project includes comprehensive testing documentation:

- Unit tests for individual components
- Integration tests with RetroPie
- Hardware tests with actual PAC devices
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
- Uses Ultimarc PAC controller hardware
- Built with libjson-c and libusb libraries

---

**Note**: The PAC LED control protocol is implemented based on general USB HID principles. For production use with actual hardware, the protocol may need fine-tuning based on:
- Specific PAC model and firmware version
- Official Ultimarc documentation
- Testing with real hardware
- USB packet analysis

The current implementation provides a solid foundation that can be adapted as needed.
