/**
 * Composable for managing button labels
 * Provides a utility to get friendly display names for button IDs
 */

import type { ButtonLabels } from '~/types'

// Reactive store for button labels loaded from config
const buttonLabels = ref<ButtonLabels>({})

/**
 * Set the button labels from config
 */
export function setButtonLabels(labels: ButtonLabels | undefined) {
    buttonLabels.value = labels || {}
}

/**
 * Get the reactive button labels ref
 */
export function getButtonLabelsRef() {
    return buttonLabels
}

/**
 * Get the friendly label for a button, with fallback to button ID
 * Format: "Player X - Label" or just the button ID if no label exists
 * 
 * @param buttonId - The technical button ID (e.g., "P1_BUTTON1")
 * @param includePlayer - Whether to include the player prefix (default: true)
 * @returns The friendly label or the original button ID
 */
export function getButtonLabel(buttonId: string, includePlayer: boolean = true): string {
    const label = buttonLabels.value[buttonId]

    if (!label) {
        // No custom label - return a default formatted name
        return formatDefaultLabel(buttonId, includePlayer)
    }

    if (includePlayer) {
        // Extract player number from button ID (e.g., "P1_BUTTON1" -> "1")
        const playerMatch = buttonId.match(/^P(\d)_/)
        if (playerMatch) {
            return `P${playerMatch[1]} - ${label}`
        }
    }

    return label
}

/**
 * Get just the custom label without player prefix, or null if no label exists
 */
export function getCustomLabel(buttonId: string): string | null {
    return buttonLabels.value[buttonId] || null
}

/**
 * Format a default label from the button ID when no custom label is set
 */
function formatDefaultLabel(buttonId: string, includePlayer: boolean): string {
    // Extract player and button type from ID (e.g., "P1_BUTTON1" -> player "1", type "BUTTON1")
    const match = buttonId.match(/^P(\d)_(.+)$/)

    if (!match) {
        return buttonId
    }

    const [, player, type] = match

    // Convert type to friendly format
    let friendlyType = type
        .replace(/(\d+)$/, ' $1')  // "BUTTON1" -> "BUTTON 1"
        .replace(/_/g, ' ')        // Any underscores to spaces
        .toLowerCase()
        .replace(/\b\w/g, c => c.toUpperCase())  // Title case

    if (includePlayer) {
        return `P${player} ${friendlyType}`
    }

    return friendlyType
}

/**
 * Composable hook for button labels
 */
export function useButtonLabels() {
    return {
        buttonLabels,
        setButtonLabels,
        getButtonLabel,
        getCustomLabel,
        getButtonLabelsRef
    }
}
