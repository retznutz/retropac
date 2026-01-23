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
      "button": "P1_BUTTON1",
      "color": "#FF0000",
      "fade": true,
      "fade_speed_ms": 500,
      "delay_ms": 0
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
| `button` | string | Yes | Button identifier (e.g., "P1_BUTTON1", "P2_START") |
| `color` | string | Yes | Target color in hex format (e.g., "#FF0000" for red) |
| `fade` | boolean | No | Whether to fade to the target color. Default: false |
| `fade_speed_ms` | integer | No | Duration of the fade in milliseconds. Only used when `fade` is true. Default: 0 |
| `delay_ms` | integer | No | Delay before this frame executes in milliseconds. 0 = immediate based on overall speed. Default: 0 |

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

### delay_ms
- `delay_ms: 0` - Frame executes immediately according to the overall `speed` setting
- `delay_ms: 100` - Frame execution is delayed by 100ms from animation start

### fade and fade_speed_ms
When `fade` is `true`:
- The button color will smoothly transition from its current color to the target `color`
- `fade_speed_ms` controls how long the transition takes (e.g., 1000 = 1 second fade)
- The fade also delays the next animation cycle completion by the fade duration

When `fade` is `false`:
- The color change is instant
- `fade_speed_ms` is ignored

## Example Animations

### Rainbow Wave
```json
{
  "name": "Rainbow Wave",
  "speed": 50,
  "loop": true,
  "frames": [
    {"button": "P1_BUTTON1", "color": "#FF0000", "fade": true, "fade_speed_ms": 200, "delay_ms": 0},
    {"button": "P1_BUTTON2", "color": "#FF7F00", "fade": true, "fade_speed_ms": 200, "delay_ms": 50},
    {"button": "P1_BUTTON3", "color": "#FFFF00", "fade": true, "fade_speed_ms": 200, "delay_ms": 100},
    {"button": "P1_BUTTON4", "color": "#00FF00", "fade": true, "fade_speed_ms": 200, "delay_ms": 150},
    {"button": "P1_BUTTON5", "color": "#0000FF", "fade": true, "fade_speed_ms": 200, "delay_ms": 200},
    {"button": "P1_BUTTON6", "color": "#8B00FF", "fade": true, "fade_speed_ms": 200, "delay_ms": 250}
  ]
}
```

### Simple Flash (No Fade)
```json
{
  "name": "Flash Red",
  "speed": 100,
  "loop": true,
  "frames": [
    {"button": "P1_BUTTON1", "color": "#FF0000", "fade": false, "fade_speed_ms": 0, "delay_ms": 0},
    {"button": "P1_BUTTON1", "color": "#000000", "fade": false, "fade_speed_ms": 0, "delay_ms": 500}
  ]
}
```

### Startup Sequence (Non-Looping)
```json
{
  "name": "Startup",
  "speed": 50,
  "loop": false,
  "frames": [
    {"button": "P1_BUTTON1", "color": "#FFFFFF", "fade": false, "fade_speed_ms": 0, "delay_ms": 0},
    {"button": "P1_BUTTON2", "color": "#FFFFFF", "fade": false, "fade_speed_ms": 0, "delay_ms": 100},
    {"button": "P1_BUTTON3", "color": "#FFFFFF", "fade": false, "fade_speed_ms": 0, "delay_ms": 200}
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
