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
    frames: AnimationFrame[]
}

export interface AnimationListItem {
    filename: string
    name: string
}
