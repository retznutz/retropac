/**
 * Animation-related type definitions
 */

export interface ButtonColorPair {
    button: string
    color: string
    controller?: number  // Controller index (0-based), defaults to 0 if not specified
}

export interface AnimationFrame {
    buttons: ButtonColorPair[]
    fade: boolean
    fade_speed_ms: number
}

export interface Animation {
    name: string
    speed: number
    loop: boolean
    hardware_fade: boolean          // Enable hardware fade for smoother transitions
    hardware_fade_rate: number      // Hardware fade rate (0=instant, 1-50=fast, 50-100=medium, 100-255=slow)
    frames: AnimationFrame[]
}

export interface AnimationListItem {
    filename: string
    name: string
}
