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
| `controller` | integer | No | Controller index (0-based) to target. Defaults to 0. Only needed for multi-controller setups. |

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
- Fades use **smooth step interpolation** (cubic Hermite) for natural-looking transitions
- **Gamma correction** (2.2) is applied for accurate brightness perception

When `fade` is `false`:
- The color change is instant
- The frame duration is determined by the `speed` setting
- `fade_speed_ms` is ignored

## Animation Smoothness

RetroPac uses several techniques to ensure smooth, professional-looking animations:

### Gamma Correction
LED brightness is non-linear to human perception. A value of 128 doesn't appear "half as bright" as 255. RetroPac applies gamma correction (γ = 2.2) to all LED output so color fades look natural.

### Smooth Step Interpolation
Instead of linear interpolation (which can look choppy), fades use smooth step:

```
smoothstep(t) = t² × (3 - 2t)
```

This creates gradual acceleration at the start and deceleration at the end of each fade.

### Built-in Animation Easing
The built-in animations also use easing:
- **Breathing**: Uses smoother step (quintic) for very natural pulsing
- **Chase**: Uses ease-out for the trailing fade effect

## Multi-Controller Support

RetroPac supports multiple PAC controllers simultaneously. When using multiple controllers, you can target specific controllers in your animation frames using the optional `controller` property.

### How Button-to-Controller Mapping Works

**ROM/Emulator Configurations (Broadcast Mode):**
- When setting button colors in ROM configs (under `emulators`), colors are sent to **all controllers**
- Each controller only responds to buttons defined in its `pin_mappings`
- Example: If `P1_BUTTON1` is mapped on controller 0 but not controller 1, only controller 0 lights up
- This means you can split buttons across controllers - P1 buttons on controller 0, P2 buttons on controller 1, etc.

**Custom Animations (Targeted Mode):**
- In animations, you can optionally specify which controller to target
- Use `"controller": 0` (or 1, 2, etc.) to target a specific controller
- Omit `controller` to broadcast to all controllers (same as ROM behavior)
- This enables animations where the same button shows different colors on different controllers

### Controller Index

- Controllers are referenced by their **0-based index** in the `ipac_controllers` array from your `config.json`
- Controller 0 is the first controller, controller 1 is the second, etc.
- If `controller` is omitted or set to -1, the button command is sent to **all controllers** (broadcast mode)

### Multi-Controller Example

```json
{
  "name": "Multi-Controller Demo",
  "speed": 300,
  "loop": true,
  "frames": [
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#FF0000", "controller": 0},
        {"button": "P1_BUTTON1", "color": "#0000FF", "controller": 1}
      ]
    },
    {
      "buttons": [
        {"button": "P1_BUTTON1", "color": "#00FF00"}
      ]
    }
  ]
}
```

In this example:
- Frame 1: Controller 0 shows red, Controller 1 shows blue on the same button
- Frame 2: Both controllers show green (no `controller` specified = all controllers)

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
