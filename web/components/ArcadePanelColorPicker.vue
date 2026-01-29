<template>
    <div class="arcade-panel-picker">
        <!-- Controller Selector -->
        <div v-if="controllerCount > 1" class="controller-selector">
            <label>Controller:</label>
            <select v-model="selectedController" class="controller-select">
                <option v-for="(name, idx) in controllerNames" :key="idx" :value="idx">
                    {{ name || `Controller ${idx + 1}` }}
                </option>
            </select>
        </div>

        <div class="arcade-panel">
            <!-- Player 1 Section -->
            <div class="player-section" v-if="hasPlayerControls(1)">
                <div class="player-label">Player 1</div>

                <!-- Start/Coin Row -->
                <div class="control-row" v-if="isConfigured('P1_COIN') || isConfigured('P1_START')">
                    <ColorButton v-if="isConfigured('P1_COIN')" button-id="P1_COIN" label="C"
                        :color="getColor('P1_COIN')" @update:color="updateColor('P1_COIN', $event)" />
                    <ColorButton v-if="isConfigured('P1_START')" button-id="P1_START" label="S"
                        :color="getColor('P1_START')" @update:color="updateColor('P1_START', $event)" />
                </div>

                <!-- Joystick -->
                <ColorButton v-if="isConfigured('P1_JOYSTICK')" button-id="P1_JOYSTICK" label="" :is-joystick="true"
                    :color="getColor('P1_JOYSTICK')" @update:color="updateColor('P1_JOYSTICK', $event)" />

                <!-- Action Buttons -->
                <div class="button-group" v-if="getConfiguredButtonCount(1) > 0" :style="getButtonGridStyle(1)">
                    <ColorButton v-for="i in getConfiguredButtonCount(1)" :key="'P1_BUTTON' + i"
                        :button-id="'P1_BUTTON' + i" :label="String(i)" :color="getColor('P1_BUTTON' + i)"
                        @update:color="updateColor('P1_BUTTON' + i, $event)" />
                </div>
            </div>

            <!-- Player 2 Section -->
            <div class="player-section" v-if="hasPlayerControls(2)">
                <div class="player-label">Player 2</div>

                <!-- Start/Coin Row -->
                <div class="control-row" v-if="isConfigured('P2_COIN') || isConfigured('P2_START')">
                    <ColorButton v-if="isConfigured('P2_COIN')" button-id="P2_COIN" label="C"
                        :color="getColor('P2_COIN')" @update:color="updateColor('P2_COIN', $event)" />
                    <ColorButton v-if="isConfigured('P2_START')" button-id="P2_START" label="S"
                        :color="getColor('P2_START')" @update:color="updateColor('P2_START', $event)" />
                </div>

                <!-- Joystick -->
                <ColorButton v-if="isConfigured('P2_JOYSTICK')" button-id="P2_JOYSTICK" label="" :is-joystick="true"
                    :color="getColor('P2_JOYSTICK')" @update:color="updateColor('P2_JOYSTICK', $event)" />

                <!-- Action Buttons -->
                <div class="button-group" v-if="getConfiguredButtonCount(2) > 0" :style="getButtonGridStyle(2)">
                    <ColorButton v-for="i in getConfiguredButtonCount(2)" :key="'P2_BUTTON' + i"
                        :button-id="'P2_BUTTON' + i" :label="String(i)" :color="getColor('P2_BUTTON' + i)"
                        @update:color="updateColor('P2_BUTTON' + i, $event)" />
                </div>
            </div>
        </div>

        <!-- Additional Controls (Players 3-4, Trackballs) - Collapsible -->
        <details v-if="hasPlayerControls(3) || hasPlayerControls(4) || hasAnyTrackballs()" style="margin-top: 1rem;">
            <summary style="cursor: pointer; color: var(--text-secondary);">
                More Controls (Players 3-4, Trackballs)
            </summary>
            <div class="arcade-panel" style="margin-top: 0.5rem;">
                <!-- Player 3 -->
                <div class="player-section" v-if="hasPlayerControls(3)">
                    <div class="player-label">Player 3</div>
                    <div class="control-row" v-if="isConfigured('P3_COIN') || isConfigured('P3_START')">
                        <ColorButton v-if="isConfigured('P3_COIN')" button-id="P3_COIN" label="C"
                            :color="getColor('P3_COIN')" @update:color="updateColor('P3_COIN', $event)" />
                        <ColorButton v-if="isConfigured('P3_START')" button-id="P3_START" label="S"
                            :color="getColor('P3_START')" @update:color="updateColor('P3_START', $event)" />
                    </div>
                    <ColorButton v-if="isConfigured('P3_JOYSTICK')" button-id="P3_JOYSTICK" label="" :is-joystick="true"
                        :color="getColor('P3_JOYSTICK')" @update:color="updateColor('P3_JOYSTICK', $event)" />
                    <div class="button-group" v-if="getConfiguredButtonCount(3) > 0" :style="getButtonGridStyle(3)">
                        <ColorButton v-for="i in getConfiguredButtonCount(3)" :key="'P3_BUTTON' + i"
                            :button-id="'P3_BUTTON' + i" :label="String(i)" :color="getColor('P3_BUTTON' + i)"
                            @update:color="updateColor('P3_BUTTON' + i, $event)" />
                    </div>
                </div>

                <!-- Player 4 -->
                <div class="player-section" v-if="hasPlayerControls(4)">
                    <div class="player-label">Player 4</div>
                    <div class="control-row" v-if="isConfigured('P4_COIN') || isConfigured('P4_START')">
                        <ColorButton v-if="isConfigured('P4_COIN')" button-id="P4_COIN" label="C"
                            :color="getColor('P4_COIN')" @update:color="updateColor('P4_COIN', $event)" />
                        <ColorButton v-if="isConfigured('P4_START')" button-id="P4_START" label="S"
                            :color="getColor('P4_START')" @update:color="updateColor('P4_START', $event)" />
                    </div>
                    <ColorButton v-if="isConfigured('P4_JOYSTICK')" button-id="P4_JOYSTICK" label="" :is-joystick="true"
                        :color="getColor('P4_JOYSTICK')" @update:color="updateColor('P4_JOYSTICK', $event)" />
                    <div class="button-group" v-if="getConfiguredButtonCount(4) > 0" :style="getButtonGridStyle(4)">
                        <ColorButton v-for="i in getConfiguredButtonCount(4)" :key="'P4_BUTTON' + i"
                            :button-id="'P4_BUTTON' + i" :label="String(i)" :color="getColor('P4_BUTTON' + i)"
                            @update:color="updateColor('P4_BUTTON' + i, $event)" />
                    </div>
                </div>

                <!-- Trackballs -->
                <div class="player-section" v-if="hasAnyTrackballs()">
                    <div class="player-label">Trackballs</div>
                    <div class="control-row">
                        <template v-for="i in 4" :key="'P' + i + '_TRACKBALL'">
                            <ColorButton v-if="isConfigured('P' + i + '_TRACKBALL')" :button-id="'P' + i + '_TRACKBALL'"
                                :label="'T' + i" :color="getColor('P' + i + '_TRACKBALL')"
                                @update:color="updateColor('P' + i + '_TRACKBALL', $event)" />
                        </template>
                    </div>
                </div>
            </div>
        </details>
    </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, h, defineComponent } from 'vue'
import { getButtonLabel } from '~/composables/useButtonLabels'

// Sub-component for color-picking button
const ColorButton = defineComponent({
    props: {
        buttonId: { type: String, required: true },
        label: { type: String, default: '' },
        color: { type: String, default: '#333333' },
        isJoystick: { type: Boolean, default: false }
    },
    emits: ['update:color'],
    setup(props, { emit }) {
        const inputRef = ref<HTMLInputElement | null>(null)

        const buttonStyle = computed(() => {
            const c = props.color && props.color !== '#000000' ? props.color : '#333'
            const style: Record<string, string> = {
                backgroundColor: c,
                boxShadow: c !== '#333' ? `0 0 15px ${c}` : 'none'
            }
            if (props.isJoystick) {
                style.width = '60px'
                style.height = '60px'
            }
            return style
        })

        const tooltip = computed(() => getButtonLabel(props.buttonId, true))

        function openPicker() {
            inputRef.value?.click()
        }

        function onColorChange(e: Event) {
            const target = e.target as HTMLInputElement
            emit('update:color', target.value)
        }

        return () => h('div', {
            style: {
                position: 'relative',
                display: 'inline-flex',
                flexDirection: 'column',
                alignItems: 'center'
            }
        }, [
            h('div', {
                class: ['arcade-button', 'color-picker-btn', props.isJoystick ? 'joystick' : ''].filter(Boolean).join(' '),
                style: buttonStyle.value,
                title: tooltip.value,
                onClick: openPicker
            }, props.label),
            h('input', {
                ref: inputRef,
                type: 'color',
                style: {
                    position: 'absolute',
                    opacity: '0',
                    width: '1px',
                    height: '1px',
                    overflow: 'hidden',
                    clip: 'rect(0, 0, 0, 0)',
                    pointerEvents: 'none'
                },
                value: props.color || '#333333',
                onInput: onColorChange
            })
        ])
    }
})

const props = defineProps<{
    /** Button colors as Record<buttonId, hexColor> */
    colors: Record<string, string>
    /** List of configured button IDs from pin_mappings per controller */
    configuredButtons: string[][]
    /** Number of controllers */
    controllerCount?: number
    /** Names of controllers (from config) */
    controllerNames?: string[]
    /** Selected controller index (v-model) */
    modelValue?: number
}>()

const emit = defineEmits<{
    (e: 'update:color', button: string, color: string, controllerIndex: number): void
    (e: 'update:modelValue', value: number): void
}>()

// Controller count (default 1)
const controllerCount = computed(() => props.controllerCount ?? 1)

// Controller names (default to array of empty strings)
const controllerNames = computed(() => {
    if (props.controllerNames && props.controllerNames.length > 0) {
        return props.controllerNames
    }
    return Array(controllerCount.value).fill('')
})

// Selected controller (internal state, synced with v-model)
const selectedController = ref(props.modelValue ?? 0)

// Watch for external v-model changes
watch(() => props.modelValue, (val) => {
    if (val !== undefined) selectedController.value = val
})

// Emit v-model updates
watch(selectedController, (val) => {
    emit('update:modelValue', val)
})

// Get configured buttons for the selected controller
const currentConfiguredButtons = computed(() => {
    if (!props.configuredButtons || props.configuredButtons.length === 0) {
        return []
    }
    return props.configuredButtons[selectedController.value] || []
})

function getColor(button: string): string {
    return props.colors[button] || '#333333'
}

function updateColor(button: string, color: string) {
    emit('update:color', button, color, selectedController.value)
}

// Helper to check if a button is configured in pin_mappings
function isConfigured(button: string): boolean {
    if (currentConfiguredButtons.value.length === 0) {
        return false
    }
    return currentConfiguredButtons.value.includes(button)
}

// Compute how many action buttons are configured for each player
function getConfiguredButtonCount(player: number): number {
    if (currentConfiguredButtons.value.length === 0) {
        return 0
    }
    let count = 0
    for (let i = 1; i <= 8; i++) {
        if (currentConfiguredButtons.value.includes(`P${player}_BUTTON${i}`)) {
            count = i
        }
    }
    return count
}

// Get the number of columns for the button grid based on count
function getButtonGridColumns(count: number): number {
    if (count <= 2) return count
    if (count <= 4) return 2
    if (count <= 6) return 3
    return 4
}

// Get dynamic grid style for button group
function getButtonGridStyle(player: number): Record<string, string> {
    const count = getConfiguredButtonCount(player)
    const cols = getButtonGridColumns(count)
    return {
        gridTemplateColumns: `repeat(${cols}, 1fr)`
    }
}

// Check if a player section has any configured controls
function hasPlayerControls(player: number): boolean {
    if (currentConfiguredButtons.value.length === 0) {
        return false
    }
    const prefix = `P${player}_`
    return currentConfiguredButtons.value.some(btn => btn.startsWith(prefix))
}

// Check if any trackballs are configured
function hasAnyTrackballs(): boolean {
    if (currentConfiguredButtons.value.length === 0) {
        return false
    }
    return currentConfiguredButtons.value.some(btn => btn.includes('_TRACKBALL'))
}
</script>

<style scoped>
.arcade-panel-picker {
    margin-bottom: 1rem;
}

.controller-selector {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    margin-bottom: 1rem;
    padding: 0 0.5rem;
}

.controller-selector label {
    font-size: 0.9rem;
    color: var(--text-secondary);
}

.controller-select {
    padding: 0.5rem 0.75rem;
    border-radius: 0.375rem;
    border: 1px solid var(--border-color);
    background: var(--bg-secondary);
    color: var(--text-primary);
    font-size: 0.875rem;
    cursor: pointer;
    min-width: 150px;
}

.controller-select:hover {
    border-color: var(--primary);
}

.controller-select:focus {
    outline: none;
    border-color: var(--primary);
}

.arcade-panel {
    background: var(--bg-tertiary);
    border-radius: 12px;
    padding: 1.5rem;
    display: flex;
    justify-content: center;
    gap: 3rem;
    border: 2px solid var(--bg-primary);
}

.player-section {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 0.75rem;
}

.player-label {
    font-size: 0.85rem;
    color: var(--text-secondary);
    margin-bottom: 0.25rem;
}

.control-row {
    display: flex;
    gap: 0.5rem;
    align-items: center;
}

.button-group {
    display: grid;
    gap: 0.5rem;
}

.arcade-button {
    width: 50px;
    height: 50px;
    border-radius: 50%;
    border: 3px solid #1f6d8f;
    cursor: pointer;
    transition: all 0.2s;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 0.7rem;
    font-weight: bold;
    color: rgba(255, 255, 255, 0.7);
    text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.5);
}

.arcade-button:hover {
    transform: scale(1.1);
    border-color: var(--accent);
}

.arcade-button.color-picker-btn {
    cursor: pointer;
    position: relative;
}

.arcade-button.color-picker-btn:hover::after {
    content: '🎨';
    position: absolute;
    font-size: 0.6rem;
    bottom: -2px;
    right: -2px;
}

.arcade-button.joystick {
    width: 60px;
    height: 60px;
}
</style>
