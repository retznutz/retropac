# Custom Animation Format

RetroPac supports custom animations defined in JSON files. This allows users to create their own LED animation sequences for arcade button lighting.

## Animation File Location

Custom animation files are stored in the `animations/` directory (configurable via `animations_dir` in `config.json`).

## File Format

Animation files use the following JSON structure:

```json
{
  "name": "Animation Display Name",
  "speed": 50,
  "loop": true,
  "frames": [
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#FF0000"},
        {"button": "P1_BUTTON2", "color": "#00FF00"}
      ],
      "fade": true,
      "fade_speed_ms": 500
    }
  ]
}
```

### Top-Level Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | No | Friendly display name for the animation. Defaults to filename if not specified. |
| `speed` | integer | No | Base timing interval in milliseconds between animation updates. Default: 50ms |
| `loop` | boolean | No | Whether the animation should loop continuously. Default: true |
| `frames` | array | Yes | Array of frame objects defining the animation sequence |

### Frame Object Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `buttons` | array | Yes | Array of button-color pairs to set in this frame |
| `fade` | boolean | No | Whether to fade to the target colors. Default: false |
| `fade_speed_ms` | integer | No | Duration of the fade in milliseconds. Only used when `fade` is true. Default: 0 |

### Button-Color Pair Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `button` | string | Yes | Button identifier (e.g., "P1_BUTTON1", "P2_START") |
| `color` | string | Yes | Target color in hex format (e.g., "#FF0000" for red) |

## Button Identifiers

Valid button identifiers include:

### Coin Buttons
- `P1_COIN`, `P2_COIN`, `P3_COIN`, `P4_COIN`

### Start Buttons
- `P1_START`, `P2_START`, `P3_START`, `P4_START`

### Player Buttons (6 per player)
- `P1_BUTTON1` through `P1_BUTTON6`
- `P2_BUTTON1` through `P2_BUTTON6`
- `P3_BUTTON1` through `P3_BUTTON6`
- `P4_BUTTON1` through `P4_BUTTON6`

### Joysticks
- `P1_JOYSTICK`, `P2_JOYSTICK`, `P3_JOYSTICK`, `P4_JOYSTICK`

### Trackballs
- `P1_TRACKBALL`, `P2_TRACKBALL`, `P3_TRACKBALL`, `P4_TRACKBALL`

## Timing Behavior

### speed
The `speed` field controls how long each frame is displayed before advancing to the next frame. For example, `speed: 100` means each frame lasts 100ms (unless fade extends it).

### fade and fade_speed_ms
When `fade` is `true`:
- The button color will smoothly transition from its current color to the target `color`
- `fade_speed_ms` controls how long the transition takes (e.g., 1000 = 1 second fade)
- The frame duration is determined by `fade_speed_ms` instead of `speed`

When `fade` is `false`:
- The color change is instant
- The frame duration is determined by the `speed` setting
- `fade_speed_ms` is ignored

## Example Animations

### Rainbow Wave
```json
{
  "name": "Rainbow Wave",
  "speed": 50,
  "loop": true,
  "frames": [
    {"buttons": [{"button": "P1_BUTTON1", "color": "#FF0000"}], "fade": true, "fade_speed_ms": 200},
    {"buttons": [{"button": "P1_BUTTON2", "color": "#FF7F00"}], "fade": true, "fade_speed_ms": 200},
    {"buttons": [{"button": "P1_BUTTON3", "color": "#FFFF00"}], "fade": true, "fade_speed_ms": 200},
    {"buttons": [{"button": "P1_BUTTON4", "color": "#00FF00"}], "fade": true, "fade_speed_ms": 200},
    {"buttons": [{"button": "P1_BUTTON5", "color": "#0000FF"}], "fade": true, "fade_speed_ms": 200},
    {"buttons": [{"button": "P1_BUTTON6", "color": "#8B00FF"}], "fade": true, "fade_speed_ms": 200}
  ]
}
```

### All Buttons Pulse (Multiple Buttons Per Frame)
```json
{
  "name": "Red Pulse",
  "speed": 100,
  "loop": true,
  "frames": [
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#FF0000"},
        {"button": "P1_BUTTON2", "color": "#FF0000"},
        {"button": "P1_BUTTON3", "color": "#FF0000"},
        {"button": "P1_BUTTON4", "color": "#FF0000"},
        {"button": "P1_BUTTON5", "color": "#FF0000"},
        {"button": "P1_BUTTON6", "color": "#FF0000"}
      ],
      "fade": true,
      "fade_speed_ms": 500
    },
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#000000"},
        {"button": "P1_BUTTON2", "color": "#000000"},
        {"button": "P1_BUTTON3", "color": "#000000"},
        {"button": "P1_BUTTON4", "color": "#000000"},
        {"button": "P1_BUTTON5", "color": "#000000"},
        {"button": "P1_BUTTON6", "color": "#000000"}
      ],
      "fade": true,
      "fade_speed_ms": 500
    }
  ]
}
```

### Startup Sequence (Non-Looping)
```json
{
  "name": "Startup",
  "speed": 100,
  "loop": false,
  "frames": [
    {"buttons": [{"button": "P1_BUTTON1", "color": "#FFFFFF"}]},
    {"buttons": [{"button": "P1_BUTTON2", "color": "#FFFFFF"}]},
    {"buttons": [{"button": "P1_BUTTON3", "color": "#FFFFFF"}]},
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#00FF00"},
        {"button": "P1_BUTTON2", "color": "#00FF00"},
        {"button": "P1_BUTTON3", "color": "#00FF00"}
      ],
      "fade": true,
      "fade_speed_ms": 1000
    }
  ]
}
```

## Configuration in config.json

To use a custom animation for idle/attract mode, add the following to your `config.json`:

```json
{
  "animations_dir": "animations",
  "idle_animation": "rainbow_wave"
}
```

### Fields

| Field | Description |
|-------|-------------|
| `animations_dir` | Directory containing animation JSON files (relative to config.json) |
| `idle_animation` | Animation filename (without .json extension) to play during attract/idle mode |

The animation filename is matched without the `.json` extension. For example, `rainbow_wave.json` is referenced as `"rainbow_wave"`.

When a ROM starts, the idle animation stops and button colors are set according to the emulator/ROM configuration. When the ROM quits, the idle animation resumes.
