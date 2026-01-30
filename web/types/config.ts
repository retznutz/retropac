/**
 * Configuration type definitions for RetroPac
 */

/**
 * RGB pin mapping for a single button
 */
export interface PinMapping {
    r_pin: number
    g_pin: number
    b_pin: number
}

/**
 * Button color configuration for a ROM or default
 * Maps button names to hex color strings
 */
export type ButtonColors = Record<string, string>

/**
 * Button label mapping for friendly display names
 * Maps button IDs (e.g., "P1_BUTTON1") to user-defined labels (e.g., "Punch")
 */
export type ButtonLabels = Record<string, string>

/**
 * iPac controller configuration
 */
export interface IpacController {
    device: string
    vendor_id: string
    product_id: string
    pin_mappings: Record<string, PinMapping>
    default?: ButtonColors
    button_labels?: ButtonLabels
}

/**
 * Controller-specific button colors for a ROM
 * Maps controller device names to their button color configurations
 */
export interface ControllerButtonColors {
    [controllerDevice: string]: ButtonColors
}

/**
 * ROM configuration - maps button colors per controller
 */
export interface RomButtonConfig {
    controllers: ControllerButtonColors
}

/**
 * ROM configuration within an emulator
 * Each ROM maps to controller-specific button configurations
 */
export interface RomConfig {
    [romName: string]: RomButtonConfig
}

/**
 * Emulator configuration with its ROMs
 */
export interface EmulatorConfig {
    roms: RomConfig
}

/**
 * Full configuration structure
 */
export interface Config {
    animations_dir: string
    ipac_controllers: IpacController[]
    emulators: Record<string, EmulatorConfig>
}

/**
 * Simplified ROM entry for UI display
 */
export interface RomEntry {
    emulator: string
    rom: string
    controllers: ControllerButtonColors
}

/**
 * Pin mapping entry for UI display
 */
export interface PinMappingEntry {
    button: string
    r_pin: number
    g_pin: number
    b_pin: number
}
