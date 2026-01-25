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
 * iPac controller configuration
 */
export interface IpacController {
    device: string
    vendor_id: string
    product_id: string
    pin_mappings: Record<string, PinMapping>
}

/**
 * Button color configuration for a ROM or default
 * Maps button names to hex color strings
 */
export type ButtonColors = Record<string, string>

/**
 * ROM configuration within an emulator
 */
export interface RomConfig {
    [romName: string]: ButtonColors
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
    default: ButtonColors
    emulators: Record<string, EmulatorConfig>
}

/**
 * Simplified ROM entry for UI display
 */
export interface RomEntry {
    emulator: string
    rom: string
    buttons: ButtonColors
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
