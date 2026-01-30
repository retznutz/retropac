<template>
    <div>
        <header class="header">
            <a href="https://github.com/retznutz/retropac" target="_blank" rel="noopener">
                <img src="~/assets/logo.svg" alt="RetroPac Animation Editor" class="header-logo" />
            </a>
            <nav class="header-nav">
                <NuxtLink to="/" class="nav-link">
                    <i class="bx bx-movie-play"></i> Animations
                </NuxtLink>
                <NuxtLink to="/config" class="nav-link active">
                    <i class="bx bx-cog"></i> Config
                </NuxtLink>
            </nav>
            <div class="btn-group">
                <button class="btn btn-secondary" @click="backupConfig" :disabled="loading">
                    <i class="bx bx-archive"></i> Backup
                </button>
                <button class="btn btn-secondary" @click="downloadConfig" :disabled="loading || !config">
                    <i class="bx bx-download"></i> Download
                </button>
                <button class="btn btn-success" @click="saveConfig" :disabled="!isDirty || loading">
                    <i class="bx bx-save"></i> Save Config
                </button>
            </div>
        </header>

        <div class="container">
            <div v-if="loading" class="empty-state">
                <i class="bx bx-loader-alt bx-spin" style="font-size: 3rem;"></i>
                <h2>Loading configuration...</h2>
            </div>

            <div v-else-if="error" class="empty-state">
                <i class="bx bx-error-circle" style="font-size: 3rem; color: var(--danger);"></i>
                <h2>{{ error }}</h2>
                <button class="btn btn-primary" @click="loadConfig">Retry</button>
            </div>

            <div v-else-if="config" class="config-layout">
                <!-- Left: Navigation Tabs -->
                <div class="config-nav card">
                    <div class="card-header">
                        <span class="card-title">Configuration</span>
                    </div>
                    <div class="config-nav-items">
                        <div class="config-nav-item" :class="{ active: activeTab === 'pins' }"
                            @click="activeTab = 'pins'">
                            <i class="bx bx-chip"></i> Pin Mappings
                        </div>
                        <div class="config-nav-item" :class="{ active: activeTab === 'defaults' }"
                            @click="activeTab = 'defaults'">
                            <i class="bx bx-palette"></i> Default Colors
                        </div>
                        <div class="config-nav-item" :class="{ active: activeTab === 'emulators' }"
                            @click="activeTab = 'emulators'">
                            <i class="bx bx-joystick"></i> Emulators & ROMs
                        </div>
                        <div class="config-nav-item" :class="{ active: activeTab === 'labels' }"
                            @click="activeTab = 'labels'">
                            <i class="bx bx-rename"></i> Button Labels
                        </div>
                    </div>
                </div>

                <!-- Right: Content Area -->
                <div class="config-content">
                    <!-- Pin Mappings Tab -->
                    <div v-if="activeTab === 'pins'" class="card">
                        <div class="card-header">
                            <span class="card-title">iPAC Controller Pin Mappings</span>
                            <button class="btn btn-primary btn-sm" @click="addController">
                                <i class="bx bx-plus"></i> Add Controller
                            </button>
                        </div>
                        <div v-if="config.ipac_controllers.length === 0" class="empty-state">
                            <p>No controllers configured</p>
                        </div>
                        <div v-else>
                            <div v-for="(controller, cIdx) in config.ipac_controllers" :key="cIdx"
                                class="controller-section">
                                <div class="controller-header">
                                    <div class="controller-info">
                                        <div class="controller-field">
                                            <label>Device:</label>
                                            <input type="text" 
                                                :value="controller.device" 
                                                @input="updateControllerDevice(cIdx, ($event.target as HTMLInputElement).value)"
                                                class="controller-input" />
                                        </div>
                                        <div class="controller-field">
                                            <label>Vendor ID:</label>
                                            <input type="text" v-model="controller.vendor_id"
                                                class="controller-input controller-input-sm" placeholder="0xd209" />
                                        </div>
                                        <div class="controller-field">
                                            <label>Product ID:</label>
                                            <input type="text" v-model="controller.product_id"
                                                class="controller-input controller-input-sm" placeholder="0x0410" />
                                        </div>
                                    </div>
                                    <button class="btn btn-danger btn-sm" @click="removeController(cIdx)">
                                        <i class="bx bx-trash"></i>
                                    </button>
                                </div>
                                <div class="pin-mappings-grid">
                                    <div v-for="(mapping, button) in controller.pin_mappings" :key="button"
                                        class="pin-mapping-card"
                                        :class="{ editing: editingPin === `${cIdx}-${button}` }">
                                        <div class="pin-button-header">
                                            <div class="pin-button-info">
                                                <div class="pin-button-name">{{ button }}</div>
                                                <div v-if="getCustomLabel(button as string)" class="pin-button-label">
                                                    {{ getCustomLabel(button as string) }}
                                                </div>
                                            </div>
                                            <div class="pin-button-actions">
                                                <button class="btn btn-primary btn-sm"
                                                    @click="testPinMapping(button as string)"
                                                    :disabled="testingButton === button"
                                                    title="Test this button on hardware">
                                                    <i
                                                        :class="testingButton === button ? 'bx bx-loader-alt bx-spin' : 'bx bx-bulb'"></i>
                                                </button>
                                                <button class="btn btn-danger btn-sm"
                                                    @click="removePinMapping(cIdx, button as string)">
                                                    <i class="bx bx-x"></i>
                                                </button>
                                            </div>
                                        </div>
                                        <div class="pin-values">
                                            <div class="pin-value">
                                                <span class="pin-label" style="color: #ff6b6b;">R</span>
                                                <input type="number" v-model.number="mapping.r_pin" min="1" max="96"
                                                    class="pin-input" />
                                            </div>
                                            <div class="pin-value">
                                                <span class="pin-label" style="color: #51cf66;">G</span>
                                                <input type="number" v-model.number="mapping.g_pin" min="1" max="96"
                                                    class="pin-input" />
                                            </div>
                                            <div class="pin-value">
                                                <span class="pin-label" style="color: #339af0;">B</span>
                                                <input type="number" v-model.number="mapping.b_pin" min="1" max="96"
                                                    class="pin-input" />
                                            </div>
                                        </div>
                                    </div>
                                    <div class="pin-mapping-card add-pin" @click="showAddPinDialog(cIdx)">
                                        <i class="bx bx-plus"></i>
                                        <span>Add Button</span>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- Default Colors Tab -->
                    <div v-if="activeTab === 'defaults'" class="card">
                        <div class="card-header">
                            <span class="card-title">Default Button Colors</span>
                            <select v-if="config.ipac_controllers.length > 1" v-model="selectedControllerIndex" class="controller-select">
                                <option v-for="(ctrl, idx) in config.ipac_controllers" :key="idx" :value="idx">
                                    {{ ctrl.device || `Controller ${idx + 1}` }}
                                </option>
                            </select>
                        </div>
                        <p class="hints">Colors used when no emulator-specific configuration exists (per controller)</p>
                        <div v-if="config.ipac_controllers.length === 0" class="empty-state">
                            <p>No controllers configured. Add a controller in the Pin Mappings tab first.</p>
                        </div>
                        <ArcadePanelColorPicker v-else
                            :colors="selectedController?.default || {}"
                            :configured-buttons="configuredButtonsPerController"
                            :controller-count="config.ipac_controllers.length"
                            :controller-names="controllerNames"
                            v-model="selectedControllerIndex"
                            @update:color="(button, color, idx) => updateDefaultColor(button, color)" />
                    </div>

                    <!-- Emulators & ROMs Tab -->
                    <div v-if="activeTab === 'emulators'" class="card">
                        <div class="card-header">
                            <span class="card-title">Emulators & ROM Configurations</span>
                            <button class="btn btn-primary btn-sm" @click="showAddEmulatorDialog">
                                <i class="bx bx-plus"></i> Add Emulator
                            </button>
                        </div>
                        <p class="hints">You must save the config before testing the buttons lights. The system reads
                            from the config
                            to show the lights.</p>
                        <div v-if="Object.keys(config.emulators).length === 0" class="empty-state">
                            <p>No emulators configured</p>
                        </div>

                        <div v-else class="emulators-list">
                            <details v-for="emulatorName in sortedEmulatorNames" :key="emulatorName"
                                class="emulator-section">
                                <summary class="emulator-header">
                                    <i class="bx bx-joystick"></i>
                                    <span class="emulator-name">{{ emulatorName }}</span>
                                    <span class="rom-count">{{ Object.keys(config.emulators[emulatorName].roms).length
                                        }} ROM(s)</span>
                                    <button class="btn btn-danger btn-sm" @click.stop="removeEmulator(emulatorName)">
                                        <i class="bx bx-trash"></i>
                                    </button>
                                </summary>

                                <div class="emulator-content">
                                    <div class="roms-header">
                                        <h4>ROMs</h4>
                                        <button class="btn btn-secondary btn-sm"
                                            @click="showAddRomDialog(emulatorName)">
                                            <i class="bx bx-plus"></i> Add ROM
                                        </button>
                                    </div>

                                    <div v-for="romName in getSortedRomNames(emulatorName)" :key="romName"
                                        class="rom-card">
                                        <div class="rom-header" @click="toggleRomExpanded(emulatorName, romName)">
                                            <i
                                                :class="isRomExpanded(emulatorName, romName) ? 'bx bx-chevron-down' : 'bx bx-chevron-right'"></i>
                                            <span class="rom-name">{{ romName }}</span>
                                            <div class="rom-actions" @click.stop>
                                                <button class="btn btn-secondary btn-sm" title="Duplicate ROM"
                                                    @click="duplicateRom(emulatorName, romName)">
                                                    <i class="bx bx-copy"></i>
                                                </button>
                                                <button class="btn btn-primary btn-sm" title="Test on hardware"
                                                    @click="testRomColors(emulatorName, romName)">
                                                    <i class="bx bx-bulb"></i>
                                                </button>
                                                <button class="btn btn-danger btn-sm"
                                                    @click="removeRom(emulatorName, romName)">
                                                    <i class="bx bx-x"></i>
                                                </button>
                                            </div>
                                        </div>

                                        <div v-if="isRomExpanded(emulatorName, romName)" class="rom-buttons">
                                            <!-- Per-controller button configuration -->
                                            <div class="rom-controllers">
                                                <div v-for="(ctrl, ctrlIdx) in config.ipac_controllers" :key="ctrl.device || ctrlIdx" 
                                                    class="rom-controller-section">
                                                    <div class="rom-controller-header">
                                                        <label class="rom-controller-toggle">
                                                            <input type="checkbox" 
                                                                :checked="isControllerEnabledForRom(emulatorName, romName, ctrl.device)"
                                                                @change="toggleControllerForRom(emulatorName, romName, ctrl.device, ($event.target as HTMLInputElement).checked)" />
                                                            <span>{{ ctrl.device || `Controller ${ctrlIdx + 1}` }}</span>
                                                        </label>
                                                    </div>
                                                    <div v-if="isControllerEnabledForRom(emulatorName, romName, ctrl.device)" 
                                                        class="rom-controller-colors">
                                                        <ArcadePanelColorPicker
                                                            :colors="getRomControllerColors(emulatorName, romName, ctrl.device)"
                                                            :configured-buttons="[configuredButtonsPerController[ctrlIdx] || []]"
                                                            :controller-count="1"
                                                            :controller-names="[ctrl.device || `Controller ${ctrlIdx + 1}`]"
                                                            @update:color="(button, color) => updateRomButtonColor(emulatorName, romName, ctrl.device, button, color)" />
                                                    </div>
                                                </div>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </details>
                        </div>
                    </div>

                    <!-- Button Labels Tab -->
                    <div v-if="activeTab === 'labels'" class="card">
                        <div class="card-header">
                            <span class="card-title">Button Labels</span>
                            <select v-if="config.ipac_controllers.length > 1" v-model="selectedControllerIndex" class="controller-select">
                                <option v-for="(ctrl, idx) in config.ipac_controllers" :key="idx" :value="idx">
                                    {{ ctrl.device || `Controller ${idx + 1}` }}
                                </option>
                            </select>
                        </div>
                        <p class="hints">Define custom friendly names for buttons. These labels are displayed in
                            tooltips and
                            selections throughout the UI (per controller).</p>
                        <div v-if="config.ipac_controllers.length === 0" class="empty-state">
                            <p>No controllers configured. Add a controller in the Pin Mappings tab first.</p>
                        </div>
                        <div v-else class="button-labels-grid">
                            <div v-for="btn in selectedControllerButtons" :key="btn" class="button-label-card">
                                <div class="button-label-header">
                                    <span class="button-id">{{ btn }}</span>
                                </div>
                                <input type="text" :value="selectedController?.button_labels?.[btn] || ''"
                                    @input="updateButtonLabel(btn, ($event.target as HTMLInputElement).value)"
                                    class="button-label-input" :placeholder="getDefaultLabelPlaceholder(btn)" />
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- Footer -->
        <footer class="footer">
            <div class="footer-content">
                <p class="footer-description">
                    <strong>RetroPac</strong> — An LED animation editor for Ultimarc I-PAC controllers. Create, preview,
                    and
                    deploy RGB lighting effects for arcade cabinet buttons.
                </p>
                <p class="footer-copyright">
                    &copy; {{ new Date().getFullYear() }} RetroPac. All rights reserved.
                </p>
            </div>
        </footer>

        <!-- Add Button Dialog -->
        <div v-if="addButtonDialog.show" class="modal-overlay" @click.self="addButtonDialog.show = false">
            <div class="modal-dialog">
                <div class="modal-header">
                    <h3>Add Button</h3>
                    <button class="btn btn-sm" @click="addButtonDialog.show = false">
                        <i class="bx bx-x"></i>
                    </button>
                </div>
                <div class="modal-body">
                    <label class="form-label">Select Button:</label>
                    <select v-model="addButtonDialog.selectedButton" class="form-select">
                        <option value="" disabled>-- Select a button --</option>
                        <option v-for="btn in availableButtonsForDialog" :key="btn" :value="btn">
                            {{ btn }}
                        </option>
                    </select>
                    <p v-if="availableButtonsForDialog.length === 0" class="no-buttons-msg">
                        All buttons are already mapped.
                    </p>
                </div>
                <div class="modal-footer">
                    <button class="btn btn-secondary" @click="addButtonDialog.show = false">Cancel</button>
                    <button class="btn btn-primary" @click="confirmAddButton"
                        :disabled="!addButtonDialog.selectedButton">
                        Add Button
                    </button>
                </div>
            </div>
        </div>

        <!-- Toast -->
        <div v-if="toast.show" class="toast" :class="'toast-' + toast.type">
            {{ toast.message }}
        </div>
    </div>
</template>

<script setup lang="ts">
import type { Config, PinMapping, ButtonColors } from '~/types'
import { setButtonLabels, getCustomLabel } from '~/composables/useButtonLabels'

const api = useApi()

const config = ref<Config | null>(null)
const loading = ref(true)
const error = ref<string | null>(null)
const isDirty = ref(false)
const activeTab = ref<'pins' | 'defaults' | 'emulators' | 'labels'>('pins')
const editingPin = ref<string | null>(null)
const expandedRoms = ref<Set<string>>(new Set())
const testingButton = ref<string | null>(null)
const selectedControllerIndex = ref(0)

const toast = ref({ show: false, message: '', type: 'success' })

// Dialog state for adding buttons
const addButtonDialog = ref({
    show: false,
    controllerIndex: -1,
    selectedButton: '',
    mode: 'pin' as 'pin' | 'default' | 'rom',
    emulatorName: '',
    romName: ''
})

let savedSnapshot = ''

// Available button names
const availableButtons = [
    'P1_COIN', 'P2_COIN', 'P3_COIN', 'P4_COIN',
    'P1_START', 'P2_START', 'P3_START', 'P4_START',
    'P1_BUTTON1', 'P1_BUTTON2', 'P1_BUTTON3', 'P1_BUTTON4', 'P1_BUTTON5', 'P1_BUTTON6', 'P1_BUTTON7', 'P1_BUTTON8',
    'P2_BUTTON1', 'P2_BUTTON2', 'P2_BUTTON3', 'P2_BUTTON4', 'P2_BUTTON5', 'P2_BUTTON6', 'P2_BUTTON7', 'P2_BUTTON8',
    'P3_BUTTON1', 'P3_BUTTON2', 'P3_BUTTON3', 'P3_BUTTON4', 'P3_BUTTON5', 'P3_BUTTON6', 'P3_BUTTON7', 'P3_BUTTON8',
    'P4_BUTTON1', 'P4_BUTTON2', 'P4_BUTTON3', 'P4_BUTTON4', 'P4_BUTTON5', 'P4_BUTTON6', 'P4_BUTTON7', 'P4_BUTTON8',
    'P1_JOYSTICK', 'P2_JOYSTICK', 'P3_JOYSTICK', 'P4_JOYSTICK',
    'P1_TRACKBALL', 'P2_TRACKBALL', 'P3_TRACKBALL', 'P4_TRACKBALL',
    'P1_LIGHTGUN', 'P2_LIGHTGUN',
    'P1_DIAL', 'P2_DIAL',
    'P1_PADDLE', 'P2_PADDLE',
    'P1_STICK', 'P2_STICK'
]

// Computed: buttons configured in pin_mappings across all controllers
const configuredButtons = computed(() => {
    if (!config.value?.ipac_controllers) return []
    const buttons: string[] = []
    for (const controller of config.value.ipac_controllers) {
        if (controller.pin_mappings) {
            buttons.push(...Object.keys(controller.pin_mappings))
        }
    }
    return [...new Set(buttons)] // Remove duplicates
})

// Computed: currently selected controller
const selectedController = computed(() => {
    if (!config.value?.ipac_controllers) return null
    return config.value.ipac_controllers[selectedControllerIndex.value] || null
})

// Computed: buttons configured for selected controller
const selectedControllerButtons = computed(() => {
    if (!selectedController.value?.pin_mappings) return []
    return Object.keys(selectedController.value.pin_mappings)
})

// Computed: buttons configured per controller (for ArcadePanelColorPicker)
const configuredButtonsPerController = computed(() => {
    if (!config.value?.ipac_controllers) return []
    return config.value.ipac_controllers.map(controller => 
        controller.pin_mappings ? Object.keys(controller.pin_mappings) : []
    )
})

// Computed: controller names (for ArcadePanelColorPicker)
const controllerNames = computed(() => {
    if (!config.value?.ipac_controllers) return []
    return config.value.ipac_controllers.map(controller => controller.device || '')
})

// Computed: available buttons for current dialog (filters out already used buttons)
const availableButtonsForDialog = computed(() => {
    if (!config.value) return availableButtons

    const dialog = addButtonDialog.value
    let usedButtons: string[] = []

    if (dialog.mode === 'pin' && dialog.controllerIndex >= 0) {
        const controller = config.value.ipac_controllers[dialog.controllerIndex]
        usedButtons = Object.keys(controller?.pin_mappings || {})
    } else if (dialog.mode === 'default') {
        usedButtons = Object.keys(selectedController.value?.default || {})
    } else if (dialog.mode === 'rom' && dialog.emulatorName && dialog.romName) {
        const rom = config.value.emulators[dialog.emulatorName]?.roms[dialog.romName]
        usedButtons = Object.keys(rom || {})
    }

    return availableButtons.filter(btn => !usedButtons.includes(btn))
})

// Computed: sorted emulator names
const sortedEmulatorNames = computed(() => {
    if (!config.value?.emulators) return []
    return Object.keys(config.value.emulators).sort((a, b) =>
        a.localeCompare(b, undefined, { numeric: true, sensitivity: 'base' })
    )
})

// Helper: get sorted ROM names for an emulator
function getSortedRomNames(emulatorName: string): string[] {
    if (!config.value?.emulators[emulatorName]?.roms) return []
    return Object.keys(config.value.emulators[emulatorName].roms).sort((a, b) =>
        a.localeCompare(b, undefined, { numeric: true, sensitivity: 'base' })
    )
}

function showToast(message: string, type: 'success' | 'error' = 'success') {
    toast.value = { show: true, message, type }
    setTimeout(() => {
        toast.value.show = false
    }, 3000)
}

async function loadConfig() {
    loading.value = true
    error.value = null

    try {
        const data = await api.getConfig()
        config.value = data
        savedSnapshot = JSON.stringify(data)
        isDirty.value = false
        
        // Merge button labels from all controllers and initialize the composable
        const mergedLabels: Record<string, string> = {}
        if (data.ipac_controllers) {
            for (const controller of data.ipac_controllers) {
                if (controller.button_labels) {
                    Object.assign(mergedLabels, controller.button_labels)
                }
            }
        }
        setButtonLabels(mergedLabels)
        
        // Reset selected controller index if needed
        if (selectedControllerIndex.value >= (data.ipac_controllers?.length || 0)) {
            selectedControllerIndex.value = 0
        }
    } catch (e) {
        error.value = 'Failed to load configuration. Make sure the server is running and config.json exists.'
    } finally {
        loading.value = false
    }
}

async function saveConfig() {
    if (!config.value || !isDirty.value) return

    try {
        await api.saveConfig(config.value)
        savedSnapshot = JSON.stringify(config.value)
        isDirty.value = false
        showToast('Configuration saved!')
    } catch (e) {
        showToast('Failed to save configuration', 'error')
    }
}

async function backupConfig() {
    try {
        const result = await api.backupConfig()
        showToast(`Backup created: ${result.filename}`)
    } catch (e) {
        showToast('Failed to create backup', 'error')
    }
}

function downloadConfig() {
    if (!config.value) return

    const json = JSON.stringify(config.value, null, 2)
    const blob = new Blob([json], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = 'config.json'
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
}

// Pin mapping functions
function addController() {
    if (!config.value) return

    config.value.ipac_controllers.push({
        device: 'new-controller',
        vendor_id: '0xd209',
        product_id: '0x0410',
        pin_mappings: {}
    })
}

// Update controller device name and propagate to all ROM references
function updateControllerDevice(controllerIndex: number, newName: string) {
    if (!config.value) return
    
    const controller = config.value.ipac_controllers[controllerIndex]
    const oldName = controller.device
    
    // Update the controller's device name
    controller.device = newName
    
    // If the name actually changed, propagate to all ROM configs
    if (oldName && oldName !== newName) {
        // Iterate through all emulators and ROMs
        for (const emulatorName in config.value.emulators) {
            const emulator = config.value.emulators[emulatorName]
            for (const romName in emulator.roms) {
                const rom = emulator.roms[romName]
                // Check if this ROM has controller configs
                if (rom.controllers && rom.controllers[oldName]) {
                    // Move the config from old name to new name
                    rom.controllers[newName] = rom.controllers[oldName]
                    delete rom.controllers[oldName]
                }
            }
        }
    }
}

function removeController(index: number) {
    if (!config.value) return
    if (!confirm('Remove this controller and all its pin mappings?')) return

    config.value.ipac_controllers.splice(index, 1)
}

function showAddPinDialog(controllerIndex: number) {
    addButtonDialog.value = {
        show: true,
        controllerIndex,
        selectedButton: '',
        mode: 'pin',
        emulatorName: '',
        romName: ''
    }
}

function confirmAddButton() {
    const dialog = addButtonDialog.value
    if (!dialog.selectedButton || !config.value) return

    if (dialog.mode === 'pin') {
        const controller = config.value.ipac_controllers[dialog.controllerIndex]
        controller.pin_mappings[dialog.selectedButton] = { r_pin: 1, g_pin: 2, b_pin: 3 }
    } else if (dialog.mode === 'default') {
        if (selectedController.value) {
            if (!selectedController.value.default) {
                selectedController.value.default = {}
            }
            selectedController.value.default[dialog.selectedButton] = '#ffffff'
        }
    } else if (dialog.mode === 'rom') {
        const rom = config.value.emulators[dialog.emulatorName]?.roms[dialog.romName]
        if (rom) {
            rom[dialog.selectedButton] = '#ffffff'
        }
    }

    dialog.show = false
}

function removePinMapping(controllerIndex: number, button: string) {
    if (!config.value) return
    if (!confirm(`Remove pin mapping for ${button}?`)) return

    delete config.value.ipac_controllers[controllerIndex].pin_mappings[button]
}

async function testPinMapping(button: string) {
    if (isDirty.value) {
        showToast('Please save the config first before testing', 'error')
        return
    }

    testingButton.value = button

    try {
        await api.testButtonLed(button, '#FFFFFF')
        showToast(`Testing ${button} - LED should be white`)

        // Turn off after 3 seconds
        setTimeout(async () => {
            try {
                await api.testButtonLed(button, '#000000')
            } catch (e) {
                // Ignore errors when turning off
            }
            testingButton.value = null
        }, 3000)
    } catch (e) {
        showToast('Failed to test button LED', 'error')
        testingButton.value = null
    }
}

// Default color functions
function updateDefaultColor(button: string, color: string) {
    if (!config.value || !selectedController.value) return
    
    // Initialize default object if it doesn't exist
    if (!selectedController.value.default) {
        selectedController.value.default = {}
    }
    
    selectedController.value.default[button] = color
}

function removeDefaultColor(button: string) {
    if (!config.value || !selectedController.value?.default) return
    if (!confirm(`Remove default color for ${button}?`)) return

    delete selectedController.value.default[button]
}

function showAddDefaultColorDialog() {
    addButtonDialog.value = {
        show: true,
        controllerIndex: -1,
        selectedButton: '',
        mode: 'default',
        emulatorName: '',
        romName: ''
    }
}

// Button label functions
function updateButtonLabel(button: string, label: string) {
    if (!config.value || !selectedController.value) return

    // Initialize button_labels if it doesn't exist
    if (!selectedController.value.button_labels) {
        selectedController.value.button_labels = {}
    }

    if (label.trim() === '') {
        // Remove the label if empty
        delete selectedController.value.button_labels[button]
    } else {
        selectedController.value.button_labels[button] = label
    }

    // Merge labels from all controllers and update the composable
    const mergedLabels: Record<string, string> = {}
    for (const controller of config.value.ipac_controllers) {
        if (controller.button_labels) {
            Object.assign(mergedLabels, controller.button_labels)
        }
    }
    setButtonLabels(mergedLabels)
}

function getDefaultLabelPlaceholder(buttonId: string): string {
    // Generate a default placeholder based on button ID
    const match = buttonId.match(/^P(\d)_(.+)$/)
    if (!match) return buttonId

    const [, , type] = match
    return type
        .replace(/(\d+)$/, ' $1')
        .replace(/_/g, ' ')
        .toLowerCase()
        .replace(/\b\w/g, c => c.toUpperCase())
}

// Emulator/ROM functions
function showAddEmulatorDialog() {
    const name = prompt('Enter emulator name (e.g., megadrive, snes):')
    if (!name) return

    if (!config.value) return

    if (config.value.emulators[name]) {
        showToast('Emulator already exists', 'error')
        return
    }

    // Create emulator with new ROM structure
    config.value.emulators[name] = {
        roms: {
            default: { controllers: {} }
        }
    }
}

function removeEmulator(name: string) {
    if (!config.value) return
    if (!confirm(`Remove emulator "${name}" and all its ROMs?`)) return

    delete config.value.emulators[name]
}

function showAddRomDialog(emulatorName: string) {
    const name = prompt('Enter ROM name (e.g., sonic, mario):')
    if (!name) return

    if (!config.value) return

    if (config.value.emulators[emulatorName].roms[name]) {
        showToast('ROM already exists', 'error')
        return
    }

    // Create ROM with new structure (empty controllers object)
    config.value.emulators[emulatorName].roms[name] = { controllers: {} }
    expandedRoms.value.add(`${emulatorName}/${name}`)
}

function removeRom(emulatorName: string, romName: string) {
    if (!config.value) return
    if (!confirm(`Remove ROM "${romName}"?`)) return

    delete config.value.emulators[emulatorName].roms[romName]
}

function duplicateRom(emulatorName: string, romName: string) {
    if (!config.value) return

    const sourceRom = config.value.emulators[emulatorName].roms[romName]
    if (!sourceRom) return

    const newName = prompt('Enter name for the duplicate ROM:', `${romName}_copy`)
    if (!newName) return

    if (config.value.emulators[emulatorName].roms[newName]) {
        showToast(`ROM "${newName}" already exists`, 'error')
        return
    }

    // Deep copy the ROM's controller configs
    config.value.emulators[emulatorName].roms[newName] = JSON.parse(JSON.stringify(sourceRom))
    expandedRoms.value.add(`${emulatorName}/${newName}`)
    showToast(`ROM "${newName}" created`)
}

async function testRomColors(emulatorName: string, romName: string) {
    if (!config.value) return

    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom?.controllers || Object.keys(rom.controllers).length === 0) {
        showToast('No button colors configured for this ROM', 'error')
        return
    }

    try {
        await api.testLeds(emulatorName, romName)
        showToast(`Testing "${romName}" on ${emulatorName}`)
    } catch (e) {
        showToast('Failed to send colors to controller', 'error')
    }
}

function toggleRomExpanded(emulatorName: string, romName: string) {
    const key = `${emulatorName}/${romName}`
    if (expandedRoms.value.has(key)) {
        expandedRoms.value.delete(key)
    } else {
        expandedRoms.value.add(key)
    }
}

function isRomExpanded(emulatorName: string, romName: string): boolean {
    return expandedRoms.value.has(`${emulatorName}/${romName}`)
}

// Check if a controller is enabled for a ROM
function isControllerEnabledForRom(emulatorName: string, romName: string, controllerDevice: string): boolean {
    if (!config.value) return false
    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom?.controllers) return false
    return controllerDevice in rom.controllers
}

// Get button colors for a specific controller in a ROM
function getRomControllerColors(emulatorName: string, romName: string, controllerDevice: string): Record<string, string> {
    if (!config.value) return {}
    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom?.controllers?.[controllerDevice]) return {}
    return rom.controllers[controllerDevice]
}

// Toggle controller on/off for a ROM
function toggleControllerForRom(emulatorName: string, romName: string, controllerDevice: string, enabled: boolean) {
    if (!config.value) return
    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom) return
    
    // Initialize controllers object if needed
    if (!rom.controllers) {
        rom.controllers = {}
    }
    
    if (enabled) {
        // Enable: add empty color config for this controller
        if (!rom.controllers[controllerDevice]) {
            rom.controllers[controllerDevice] = {}
        }
    } else {
        // Disable: remove this controller's config
        delete rom.controllers[controllerDevice]
    }
}

// Update ROM button color for a specific controller
function updateRomButtonColor(emulatorName: string, romName: string, controllerDevice: string, button: string, color: string) {
    if (!config.value) return
    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom) return
    
    // Ensure controllers structure exists
    if (!rom.controllers) {
        rom.controllers = {}
    }
    if (!rom.controllers[controllerDevice]) {
        rom.controllers[controllerDevice] = {}
    }
    
    rom.controllers[controllerDevice][button] = color
}

function removeRomButton(emulatorName: string, romName: string, controllerDevice: string, button: string) {
    if (!config.value) return
    const rom = config.value.emulators[emulatorName]?.roms[romName]
    if (!rom?.controllers?.[controllerDevice]) return
    delete rom.controllers[controllerDevice][button]
}

function showAddRomButtonDialog(emulatorName: string, romName: string) {
    addButtonDialog.value = {
        show: true,
        controllerIndex: -1,
        selectedButton: '',
        mode: 'rom',
        emulatorName,
        romName
    }
}

// Watch for changes
watch(config, (newVal) => {
    if (newVal && savedSnapshot) {
        isDirty.value = JSON.stringify(newVal) !== savedSnapshot
    }
}, { deep: true })

// Warn before leaving with unsaved changes
function handleBeforeUnload(e: BeforeUnloadEvent) {
    if (isDirty.value) {
        e.preventDefault()
        e.returnValue = 'You have unsaved changes. Are you sure you want to leave?'
        return e.returnValue
    }
}

onMounted(() => {
    loadConfig()
    window.addEventListener('beforeunload', handleBeforeUnload)
})

onBeforeUnmount(() => {
    window.removeEventListener('beforeunload', handleBeforeUnload)
})
</script>
