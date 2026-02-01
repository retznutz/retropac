<template>
    <div>
        <header class="header">
            <a href="https://github.com/retznutz/retropac" target="_blank" rel="noopener">
                <img src="~/assets/logo.svg" alt="RetroPac Animation Editor" class="header-logo" />
            </a>
            <nav class="header-nav">
                <NuxtLink to="/" class="nav-link active">
                    <i class="bx bx-movie-play"></i> Animations
                </NuxtLink>
                <NuxtLink to="/config" class="nav-link">
                    <i class="bx bx-cog"></i> Config
                </NuxtLink>
            </nav>
            <div class="btn-group">
                <button v-if="!isPlaying" class="btn btn-primary" @click="startPreview"
                    :disabled="!currentAnimation || currentAnimation.frames.length === 0">
                    <i class="bx bx-play"></i> Preview
                </button>
                <button v-else class="btn btn-warning" @click="stopPreview">
                    <i class="bx bx-stop"></i> Stop
                </button>
                <button class="btn btn-success" @click="saveAnimation" :disabled="!currentAnimation || isPlaying">
                    <i class="bx bx-save"></i> Save
                </button>
                <button v-if="!isPlayingOnHardware" class="btn btn-accent" @click="playOnHardware"
                    :disabled="!currentAnimation || !currentAnimationName || isDirty">
                    <i class="bx bx-chip"></i> Play on Hardware
                </button>
                <button v-else class="btn btn-warning" @click="stopOnHardware">
                    <i class="bx bx-stop"></i> Stop Hardware
                </button>
                <button class="btn btn-secondary" @click="setAsAttractMode"
                    :disabled="!currentAnimation || !currentAnimationName || isDirty">
                    <i class="bx bx-star"></i> Set as Attract
                </button>
            </div>
        </header>

        <div class="container">
            <div class="editor-layout">
                <!-- Animation List Sidebar -->
                <div class="card">
                    <div class="card-header">
                        <span class="card-title">Animations</span>
                        <button class="btn btn-primary btn-sm" @click="createNewAnimation"><i class="bx bx-plus"></i>
                            New</button>
                    </div>
                    <div class="animation-list">
                        <div v-for="anim in animations" :key="anim.filename" class="animation-item"
                            :class="{ active: currentAnimationName === anim.filename }"
                            @click="loadAnimation(anim.filename)">
                            <div class="animation-item-info">
                                <span class="animation-friendly-name">{{ anim.name }}</span>
                                <span class="animation-filename">{{ anim.filename }}</span>
                            </div>
                            <div class="animation-item-actions">
                                <button class="btn btn-secondary btn-sm" @click.stop="duplicateAnimation(anim.filename)"
                                    title="Duplicate animation"><i class="bx bx-copy"></i></button>
                                <button class="btn btn-danger btn-sm" @click.stop="deleteAnimation(anim.filename)"><i
                                        class="bx bx-x"></i></button>
                            </div>
                        </div>
                        <div v-if="animations.length === 0" class="empty-state">
                            <p>No animations yet</p>
                        </div>
                    </div>
                </div>

                <!-- Main Editor Area -->
                <div class="card">
                    <div class="card-header">
                        <span class="card-title">{{ currentAnimation?.name || 'Select an animation' }}</span>
                        <p v-if="currentAnimation != null" class="hints">Use the Config > Pin Mappings to add buttons
                            here.</p>
                    </div>

                    <div v-if="currentAnimation">
                        <!-- Arcade Panel Preview -->
                        <ArcadePanel :buttons="isPlaying ? previewButtons : (selectedFrame?.buttons || [])"
                            :selected-buttons="isPlaying ? [] : selectedButtons" :configured-buttons="configuredButtons"
                            :controller-count="controllerCount" :controller-names="controllerNames"
                            v-model="selectedControllerIndex" @button-click="toggleButton" />
                        <div v-if="isPlaying" class="preview-indicator">
                            <i class="bx bx-radio-circle-marked bx-flashing"></i> Playing Frame {{ previewFrameIndex + 1
                            }} / {{
                                currentAnimation.frames.length }}
                        </div>

                        <!-- Timeline -->
                        <AnimationTimeline :frames="currentAnimation.frames" :selected-index="selectedFrameIndex"
                            :selected-indices="selectedFrameIndices" :is-playing="isPlaying"
                            :playing-index="previewFrameIndex"
                            @select-frame="(index, e) => selectFrame(index, e.ctrlKey, e.shiftKey, e.metaKey)"
                            @clear-selection="clearFrameSelection" @duplicate-frame="duplicateFrame"
                            @remove-frame="removeFrame" @add-frame="addFrame" @reorder-frames="reorderFrames" />
                    </div>

                    <div v-else class="empty-state">
                        <div class="empty-state-icons">
                            <div class="empty-state-icon" style="opacity: 1"><i class="bx bx-run"></i></div>
                            <div class="empty-state-icon" style="opacity: 0.7"><i class="bx bx-run"></i></div>
                            <div class="empty-state-icon" style="opacity: 0.4"><i class="bx bx-run"></i></div>
                            <div class="empty-state-icon" style="opacity: 0.2"><i class="bx bx-run"></i></div>
                        </div>
                        <h2>Select an animation from the list or create a new one</h2>
                    </div>
                </div>

                <!-- Properties Panel -->
                <div class="properties-panel">
                    <!-- Animation Properties -->
                    <div class="card" v-if="currentAnimation">
                        <div class="card-header">
                            <span class="card-title">Animation Settings</span>
                            <button class="btn btn-secondary btn-sm" @click="downloadAnimation" title="Download JSON">
                                <i class="bx bx-download"></i>
                            </button>
                        </div>
                        <div class="form-group">
                            <label>Name</label>
                            <input type="text" class="form-control" v-model="currentAnimation.name" />
                        </div>
                        <div class="form-group">
                            <label>File Name</label>
                            <div class="filename-input-group">
                                <input type="text" class="form-control" v-model="editableFilename"
                                    @blur="renameAnimation" @keyup.enter="renameAnimation" />
                            </div>
                        </div>
                        <div class="form-group">
                            <label>Speed (ms)</label>
                            <input type="number" class="form-control" v-model.number="currentAnimation.speed"
                                min="10" />
                        </div>
                        <div class="form-group">
                            <div class="form-check">
                                <input type="checkbox" id="loop" v-model="currentAnimation.loop" />
                                <label for="loop">Loop Animation</label>
                            </div>
                        </div>

                        <!-- Hardware Fade Settings -->
                        <div class="form-group"
                            style="margin-top: 1rem; padding-top: 1rem; border-top: 1px solid var(--border-color);">
                            <div class="form-check">
                                <input type="checkbox" id="hardware-fade" v-model="currentAnimation.hardware_fade" />
                                <label for="hardware-fade">Enable Hardware Fade</label>
                            </div>
                            <p style="font-size: 0.75rem; color: var(--text-secondary); margin-top: 0.25rem;">
                                Uses I-PAC hardware for smoother LED transitions
                            </p>
                        </div>
                        <div class="form-group" v-if="currentAnimation.hardware_fade">
                            <label>Hardware Fade Rate</label>
                            <div style="display: flex; align-items: center; gap: 0.5rem;">
                                <input type="range" class="form-range"
                                    v-model.number="currentAnimation.hardware_fade_rate" min="1" max="15" step="1"
                                    style="flex: 1;" />
                                <span style="min-width: 3rem; text-align: right;">{{ currentAnimation.hardware_fade_rate
                                }}</span>
                            </div>
                            <div
                                style="display: flex; justify-content: space-between; font-size: 0.7rem; color: var(--text-secondary);">
                                <span>Fast (1)</span>
                                <span>Medium (8)</span>
                                <span>Slow (15)</span>
                            </div>
                        </div>
                    </div>

                    <!-- Frame Properties (Multi-selection) -->
                    <div class="card" v-if="hasMultipleFramesSelected">
                        <div class="card-header">
                            <span class="card-title">{{ selectedFrameIndices.length }} Frames Selected</span>
                        </div>
                        <p style="font-size: 0.85rem; color: var(--text-secondary); margin-bottom: 0.5rem;">
                            Frames: {{selectedFrameIndices.map(i => i + 1).join(', ')}}
                        </p>
                        <div class="form-group">
                            <div class="form-check">
                                <input type="checkbox" id="fade-multi" :checked="multiFrameFadeValue === true"
                                    :indeterminate="multiFrameFadeValue === null"
                                    @change="applyFadeToSelectedFrames(($event.target as HTMLInputElement).checked)" />
                                <label for="fade-multi">Enable Fade</label>
                            </div>
                        </div>
                        <div class="form-group">
                            <label>Fade Speed (ms)</label>
                            <input type="number" class="form-control" :value="multiFrameFadeSpeedValue ?? ''"
                                :placeholder="multiFrameFadeSpeedValue === null ? 'Mixed' : ''"
                                @input="applyFadeSpeedToSelectedFrames(parseInt(($event.target as HTMLInputElement).value) || 0)"
                                min="0" />
                        </div>
                    </div>

                    <!-- Frame Properties (Single selection) -->
                    <div class="card" v-else-if="selectedFrame">
                        <div class="card-header">
                            <span class="card-title">Frame {{ selectedFrameIndex + 1 }} Settings</span>
                        </div>
                        <div class="form-group">
                            <div class="form-check">
                                <input type="checkbox" id="fade" v-model="selectedFrame.fade" />
                                <label for="fade">Enable Fade</label>
                            </div>
                        </div>
                        <div class="form-group" v-if="selectedFrame.fade">
                            <label>Fade Speed (ms)</label>
                            <input type="number" class="form-control" v-model.number="selectedFrame.fade_speed_ms"
                                min="0" />
                        </div>
                    </div>

                    <!-- Button Color -->
                    <div class="card" v-if="selectedButtons.length > 0">
                        <div class="card-header">
                            <span class="card-title">Button Color</span>
                        </div>
                        <p style="font-size: 0.85rem; color: var(--text-secondary); margin-bottom: 0.5rem;">
                            Selected: {{ selectedButtonLabels }}
                        </p>
                        <div class="color-picker-wrapper">
                            <div class="color-preview color-preview-clickable"
                                :style="{ backgroundColor: selectedColor }" @click="openColorPicker"
                                title="Click to pick color"></div>
                            <input ref="colorInputRef" type="color" v-model="selectedColor" @input="applyColor"
                                class="color-input-hidden" />
                            <input type="text" v-model="selectedColor" @input="applyColor" class="color-hex-input" />
                        </div>
                        <div class="btn-group" style="margin-top: 0.5rem;">
                            <button class="btn btn-danger btn-sm" @click="removeSelectedButtons">Remove</button>
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

        <!-- Toast -->
        <div v-if="toast.show" class="toast" :class="'toast-' + toast.type">
            {{ toast.message }}
        </div>
    </div>
</template>

<script setup lang="ts">
import type { ButtonColorPair, AnimationFrame, AnimationListItem, Animation } from '~/types'
import { setButtonLabels, getButtonLabel } from '~/composables/useButtonLabels'

const api = useApi()

const animations = ref<AnimationListItem[]>([])
const currentAnimationName = ref<string | null>(null)
const currentAnimation = ref<Animation | null>(null)
const selectedFrameIndex = ref(0)
const selectedFrameIndices = ref<number[]>([])
const selectedButtons = ref<string[]>([])
const selectedColor = ref('#FF0000')
const editableFilename = ref('')
const colorInputRef = ref<HTMLInputElement | null>(null)

// Playback preview state
const isPlaying = ref(false)
const isPlayingOnHardware = ref(false)
const previewFrameIndex = ref(0)
const previewButtons = ref<ButtonColorPair[]>([])
let playbackTimeout: ReturnType<typeof setTimeout> | null = null
let fadeStartTime = 0
let fadeAnimationFrame: number | null = null

const toast = ref({ show: false, message: '', type: 'success' })

// Configured buttons from pin_mappings (per controller)
const configuredButtons = ref<string[][]>([])
const controllerCount = ref(1)
const controllerNames = ref<string[]>([])
const selectedControllerIndex = ref(0)

// Dirty state tracking
const isDirty = ref(false)
let savedSnapshot = ''

const selectedFrame = computed(() => {
    if (!currentAnimation.value || selectedFrameIndex.value < 0) return null
    return currentAnimation.value.frames[selectedFrameIndex.value]
})

// Computed: check if multiple frames are selected
const hasMultipleFramesSelected = computed(() => selectedFrameIndices.value.length > 1)

// Computed: get all selected frames
const selectedFrames = computed(() => {
    if (!currentAnimation.value) return []
    return selectedFrameIndices.value
        .filter(i => i >= 0 && i < currentAnimation.value!.frames.length)
        .map(i => currentAnimation.value!.frames[i])
})

// Computed: check if all selected frames have the same fade setting
const multiFrameFadeValue = computed(() => {
    const frames = selectedFrames.value
    if (frames.length === 0) return false
    const firstFade = frames[0].fade
    return frames.every(f => f.fade === firstFade) ? firstFade : null
})

// Computed: check if all selected frames have the same fade_speed_ms
const multiFrameFadeSpeedValue = computed(() => {
    const frames = selectedFrames.value
    if (frames.length === 0) return 0
    const firstSpeed = frames[0].fade_speed_ms
    return frames.every(f => f.fade_speed_ms === firstSpeed) ? firstSpeed : null
})

// Apply fade setting to all selected frames
function applyFadeToSelectedFrames(fade: boolean) {
    if (!currentAnimation.value) return
    selectedFrameIndices.value.forEach(i => {
        if (i >= 0 && i < currentAnimation.value!.frames.length) {
            currentAnimation.value!.frames[i].fade = fade
        }
    })
}

// Apply fade speed to all selected frames
function applyFadeSpeedToSelectedFrames(speed: number) {
    if (!currentAnimation.value) return
    selectedFrameIndices.value.forEach(i => {
        if (i >= 0 && i < currentAnimation.value!.frames.length) {
            currentAnimation.value!.frames[i].fade_speed_ms = speed
        }
    })
}

// Computed: selected button labels for display
const selectedButtonLabels = computed(() => {
    return selectedButtons.value.map(btn => getButtonLabel(btn, true)).join(', ')
})

// Show toast notification
function showToast(message: string, type: 'success' | 'error' = 'success') {
    toast.value = { show: true, message, type }
    setTimeout(() => {
        toast.value.show = false
    }, 3000)
}

// Load animation list
async function loadAnimationList() {
    try {
        const data = await api.getAnimations()
        animations.value = data.animations || []
    } catch (e) {
        showToast('Failed to load animations', 'error')
    }
}

// Load specific animation
async function loadAnimation(name: string) {
    if (isDirty.value) {
        if (!confirm('You have unsaved changes. Discard and load another animation?')) {
            return
        }
    }

    try {
        const data = await api.getAnimation(name)
        // Ensure hardware_fade fields have defaults for backwards compatibility
        if (data.hardware_fade === undefined) {
            data.hardware_fade = true
        }
        if (data.hardware_fade_rate === undefined) {
            data.hardware_fade_rate = 50
        }
        currentAnimation.value = data
        currentAnimationName.value = name
        editableFilename.value = name
        selectedFrameIndex.value = 0
        selectedFrameIndices.value = [0]
        selectedButtons.value = []
        savedSnapshot = JSON.stringify(data)
        isDirty.value = false
    } catch (e) {
        showToast('Failed to load animation', 'error')
    }
}

// Save current animation
async function saveAnimation() {
    if (!currentAnimation.value || !currentAnimationName.value) return

    try {
        // Use filename from current name, sanitize it
        const filename = currentAnimationName.value.replace(/[^a-zA-Z0-9_-]/g, '_')
        await api.saveAnimation(filename, currentAnimation.value)
        savedSnapshot = JSON.stringify(currentAnimation.value)
        isDirty.value = false
        showToast('Animation saved!')
        await loadAnimationList()
    } catch (e) {
        showToast('Failed to save animation', 'error')
    }
}

// Download animation as JSON file
function downloadAnimation() {
    if (!currentAnimation.value || !currentAnimationName.value) return

    const json = JSON.stringify(currentAnimation.value, null, 2)
    const blob = new Blob([json], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `${currentAnimationName.value}.json`
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
}

// Create new animation
function createNewAnimation() {
    if (isDirty.value) {
        if (!confirm('You have unsaved changes. Discard and create a new animation?')) {
            return
        }
    }

    const name = prompt('Enter animation name:')
    if (!name) return

    currentAnimation.value = {
        name,
        speed: 100,
        loop: true,
        hardware_fade: true,
        hardware_fade_rate: 50,
        frames: [
            {
                buttons: [],
                fade: false,
                fade_speed_ms: 0
            }
        ]
    }
    currentAnimationName.value = name.replace(/[^a-zA-Z0-9_-]/g, '_')
    editableFilename.value = currentAnimationName.value
    selectedFrameIndex.value = 0
    selectedFrameIndices.value = [0]
    selectedButtons.value = []
    savedSnapshot = JSON.stringify(currentAnimation.value)
    isDirty.value = false
}

// Delete animation
async function deleteAnimation(name: string) {
    if (!confirm(`Delete animation "${name}"?`)) return

    try {
        await api.deleteAnimation(name)
        showToast('Animation deleted')
        if (currentAnimationName.value === name) {
            currentAnimation.value = null
            currentAnimationName.value = null
        }
        await loadAnimationList()
    } catch (e) {
        showToast('Failed to delete animation', 'error')
    }
}

// Duplicate animation
async function duplicateAnimation(name: string) {
    try {
        const response = await api.duplicateAnimation(name)
        if (response.success) {
            showToast(`Duplicated to ${response.duplicate}`)
            await loadAnimationList()
            // Load the new duplicate
            await loadAnimation(response.duplicate)
        } else {
            showToast(response.error || 'Failed to duplicate animation', 'error')
        }
    } catch (e) {
        showToast('Failed to duplicate animation', 'error')
    }
}

// Rename animation file
async function renameAnimation() {
    if (!currentAnimationName.value) return

    // Sanitize the filename
    const newFilename = editableFilename.value.replace(/[^a-zA-Z0-9_-]/g, '_')
    editableFilename.value = newFilename

    // Skip if name hasn't changed
    if (newFilename === currentAnimationName.value) return

    if (!newFilename) {
        editableFilename.value = currentAnimationName.value
        return
    }

    try {
        const response = await api.renameAnimation(currentAnimationName.value, newFilename)
        if (response.success) {
            currentAnimationName.value = newFilename
            showToast(`Renamed to ${newFilename}`)
            await loadAnimationList()
        } else {
            editableFilename.value = currentAnimationName.value
            showToast(response.error || 'Failed to rename animation', 'error')
        }
    } catch (e) {
        editableFilename.value = currentAnimationName.value
        showToast('Failed to rename animation', 'error')
    }
}

// Frame management
function selectFrame(index: number, ctrlKey = false, shiftKey = false, metaKey = false) {
    const multiSelectKey = ctrlKey || metaKey

    if (shiftKey && selectedFrameIndices.value.length > 0) {
        // Shift+click: range selection from last selected to clicked
        const lastSelected = selectedFrameIndices.value[selectedFrameIndices.value.length - 1]
        const start = Math.min(lastSelected, index)
        const end = Math.max(lastSelected, index)
        const range: number[] = []
        for (let i = start; i <= end; i++) {
            range.push(i)
        }
        // Merge with existing selection (keep unique)
        const merged = [...new Set([...selectedFrameIndices.value, ...range])]
        selectedFrameIndices.value = merged.sort((a, b) => a - b)
    } else if (multiSelectKey) {
        // Ctrl/Cmd+click: toggle selection
        const idx = selectedFrameIndices.value.indexOf(index)
        if (idx === -1) {
            selectedFrameIndices.value = [...selectedFrameIndices.value, index].sort((a, b) => a - b)
        } else {
            selectedFrameIndices.value = selectedFrameIndices.value.filter(i => i !== index)
        }
    } else {
        // Normal click: single selection
        selectedFrameIndices.value = [index]
    }

    // Update the primary selected frame for editing
    selectedFrameIndex.value = index
    selectedButtons.value = []
}

function clearFrameSelection() {
    selectedFrameIndices.value = selectedFrameIndex.value >= 0 ? [selectedFrameIndex.value] : []
}

function addFrame() {
    if (!currentAnimation.value) return

    currentAnimation.value.frames.push({
        buttons: [],
        fade: false,
        fade_speed_ms: 0
    })
    selectedFrameIndex.value = currentAnimation.value.frames.length - 1
}

function duplicateFrame(index: number) {
    if (!currentAnimation.value) return

    const sourceFrame = currentAnimation.value.frames[index]
    const newFrame = {
        buttons: sourceFrame.buttons.map(btn => ({ ...btn })),
        fade: sourceFrame.fade,
        fade_speed_ms: sourceFrame.fade_speed_ms
    }

    // Insert after the current frame
    currentAnimation.value.frames.splice(index + 1, 0, newFrame)
    selectedFrameIndex.value = index + 1
}

function removeFrame(index: number) {
    if (!currentAnimation.value || currentAnimation.value.frames.length <= 1) return
    if (!confirm(`Delete frame ${index + 1}?`)) return

    currentAnimation.value.frames.splice(index, 1)
    if (selectedFrameIndex.value >= currentAnimation.value.frames.length) {
        selectedFrameIndex.value = currentAnimation.value.frames.length - 1
    }
    // Update multi-selection: remove the deleted index and adjust remaining
    selectedFrameIndices.value = selectedFrameIndices.value
        .filter(i => i !== index)
        .map(i => i > index ? i - 1 : i)
    if (selectedFrameIndices.value.length === 0 && currentAnimation.value.frames.length > 0) {
        selectedFrameIndices.value = [selectedFrameIndex.value]
    }
}

// Reorder frames (from timeline component)
function reorderFrames(fromIndex: number, toIndex: number) {
    if (!currentAnimation.value) return

    const frames = currentAnimation.value.frames
    const [movedFrame] = frames.splice(fromIndex, 1)
    frames.splice(toIndex, 0, movedFrame)

    // Update selected frame index to follow the moved frame if it was selected
    if (selectedFrameIndex.value === fromIndex) {
        selectedFrameIndex.value = toIndex
    } else if (selectedFrameIndex.value > fromIndex && selectedFrameIndex.value <= toIndex) {
        selectedFrameIndex.value--
    } else if (selectedFrameIndex.value < fromIndex && selectedFrameIndex.value >= toIndex) {
        selectedFrameIndex.value++
    }

    // Update multi-selection indices to follow moved frames
    selectedFrameIndices.value = selectedFrameIndices.value.map(i => {
        if (i === fromIndex) return toIndex
        if (fromIndex < toIndex) {
            if (i > fromIndex && i <= toIndex) return i - 1
        } else {
            if (i >= toIndex && i < fromIndex) return i + 1
        }
        return i
    }).sort((a, b) => a - b)
}

// Button selection and color
function toggleButton(button: string, controllerIndex: number) {
    const idx = selectedButtons.value.indexOf(button)
    if (idx >= 0) {
        selectedButtons.value.splice(idx, 1)
    } else {
        selectedButtons.value.push(button)

        // Get current color if button already has one (matching controller)
        const frame = selectedFrame.value
        if (frame) {
            const existing = frame.buttons.find(b =>
                b.button === button &&
                (b.controller === undefined || b.controller === controllerIndex)
            )
            if (existing) {
                selectedColor.value = existing.color
            }
        }
    }
}

function applyColor() {
    if (!selectedFrame.value || selectedButtons.value.length === 0) return

    for (const button of selectedButtons.value) {
        // Find existing button entry for this controller
        const existing = selectedFrame.value.buttons.find(b =>
            b.button === button &&
            (b.controller === undefined || b.controller === selectedControllerIndex.value)
        )
        if (existing) {
            existing.color = selectedColor.value
            // Ensure controller is set
            if (controllerCount.value > 1 && existing.controller === undefined) {
                existing.controller = selectedControllerIndex.value
            }
        } else {
            const newEntry: { button: string; color: string; controller?: number } = {
                button,
                color: selectedColor.value
            }
            // Only add controller property if there are multiple controllers
            if (controllerCount.value > 1) {
                newEntry.controller = selectedControllerIndex.value
            }
            selectedFrame.value.buttons.push(newEntry)
        }
    }
}

function openColorPicker() {
    colorInputRef.value?.click()
}

function removeSelectedButtons() {
    if (!selectedFrame.value) return

    // Remove buttons for the currently selected controller
    selectedFrame.value.buttons = selectedFrame.value.buttons.filter(
        b => !(selectedButtons.value.includes(b.button) &&
            (b.controller === undefined || b.controller === selectedControllerIndex.value))
    )
    selectedButtons.value = []
}

// Preview playback
function startPreview() {
    if (!currentAnimation.value || currentAnimation.value.frames.length === 0) return

    isPlaying.value = true
    previewFrameIndex.value = 0
    previewButtons.value = [] // Start with all buttons off
    playNextFrame()
}

function stopPreview() {
    isPlaying.value = false
    if (playbackTimeout) {
        clearTimeout(playbackTimeout)
        playbackTimeout = null
    }
    if (fadeAnimationFrame) {
        cancelAnimationFrame(fadeAnimationFrame)
        fadeAnimationFrame = null
    }
    previewButtons.value = []
}

async function playOnHardware() {
    if (!currentAnimationName.value || isDirty.value) return

    try {
        const response = await api.playAnimation(currentAnimationName.value)
        if (response.success) {
            isPlayingOnHardware.value = true
            showToast('Animation started on hardware', 'success')
        } else {
            showToast(response.error || 'Failed to start animation', 'error')
        }
    } catch (error) {
        showToast('Failed to start animation on hardware', 'error')
    }
}

async function stopOnHardware() {
    try {
        const response = await api.stopAnimation()
        if (response.success) {
            isPlayingOnHardware.value = false
            showToast('Animation stopped', 'success')
        } else {
            showToast(response.error || 'Failed to stop animation', 'error')
        }
    } catch (error) {
        showToast('Failed to stop animation', 'error')
    }
}

async function setAsAttractMode() {
    if (!currentAnimationName.value || isDirty.value) return

    if (!confirm(`Set "${currentAnimationName.value}" as the attract mode animation?\n\nThis will update autostart.sh and runcommand-onend.sh on RetroPie.`)) {
        return
    }

    try {
        const response = await api.setAttractMode(currentAnimationName.value)
        if (response.success) {
            showToast(response.message || 'Attract mode updated!', 'success')
        } else {
            showToast(response.error || 'Failed to update attract mode', 'error')
        }
    } catch (error) {
        showToast('Failed to update attract mode', 'error')
    }
}

function playNextFrame() {
    if (!isPlaying.value || !currentAnimation.value) return

    const anim = currentAnimation.value
    const frame = anim.frames[previewFrameIndex.value]

    if (frame.fade && frame.fade_speed_ms > 0) {
        // Animate fade
        const startColors = new Map<string, string>()
        for (const btn of previewButtons.value) {
            startColors.set(btn.button, btn.color)
        }

        fadeStartTime = performance.now()

        function animateFade() {
            if (!isPlaying.value || !currentAnimation.value) return

            const elapsed = performance.now() - fadeStartTime
            const progress = Math.min(elapsed / frame.fade_speed_ms, 1)

            // Build new button list - only buttons in this frame
            const newButtons: ButtonColorPair[] = []
            for (const target of frame.buttons) {
                const startColor = startColors.get(target.button) || '#000000'
                const interpolated = lerpColor(startColor, target.color, progress)
                newButtons.push({ button: target.button, color: interpolated })
            }
            previewButtons.value = newButtons

            if (progress < 1) {
                fadeAnimationFrame = requestAnimationFrame(animateFade)
            } else {
                fadeAnimationFrame = null
                scheduleNextFrame()
            }
        }

        fadeAnimationFrame = requestAnimationFrame(animateFade)
    } else {
        // Instant color change - start with all buttons off, then apply frame colors
        const newButtons: ButtonColorPair[] = []

        // Keep buttons that are in this frame with their new colors
        for (const btn of frame.buttons) {
            newButtons.push({ ...btn })
        }

        previewButtons.value = newButtons

        // Schedule next frame after speed delay
        playbackTimeout = setTimeout(() => {
            scheduleNextFrame()
        }, anim.speed)
    }
}

function scheduleNextFrame() {
    if (!isPlaying.value || !currentAnimation.value) return

    const anim = currentAnimation.value
    previewFrameIndex.value++

    if (previewFrameIndex.value >= anim.frames.length) {
        if (anim.loop) {
            previewFrameIndex.value = 0
            playNextFrame()
        } else {
            stopPreview()
        }
    } else {
        playNextFrame()
    }
}

function lerpColor(from: string, to: string, t: number): string {
    // Apply smooth-step easing for more natural transitions (matches C implementation)
    t = smoothStep(t)

    const fromRgb = hexToRgb(from)
    const toRgb = hexToRgb(to)
    const r = Math.round(fromRgb.r + (toRgb.r - fromRgb.r) * t)
    const g = Math.round(fromRgb.g + (toRgb.g - fromRgb.g) * t)
    const b = Math.round(fromRgb.b + (toRgb.b - fromRgb.b) * t)
    return rgbToHex(r, g, b)
}

// Easing functions to match hardware animation (same as C implementation)
function smoothStep(t: number): number {
    return t * t * (3 - 2 * t)
}

function hexToRgb(hex: string): { r: number, g: number, b: number } {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex)
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : { r: 0, g: 0, b: 0 }
}

function rgbToHex(r: number, g: number, b: number): string {
    return '#' + [r, g, b].map(x => x.toString(16).padStart(2, '0')).join('')
}

// Watch for changes to mark as dirty
watch(currentAnimation, (newVal) => {
    if (newVal && savedSnapshot) {
        isDirty.value = JSON.stringify(newVal) !== savedSnapshot
    }
}, { deep: true })

// Warn before closing browser with unsaved changes
function handleBeforeUnload(e: BeforeUnloadEvent) {
    if (isDirty.value) {
        e.preventDefault()
        e.returnValue = 'You have unsaved changes. Are you sure you want to leave?'
        return e.returnValue
    }
}

// Load button labels and configured buttons from config
async function loadConfigData() {
    try {
        const config = await api.getConfig()

        // Merge button labels from all controllers
        const mergedLabels: Record<string, string> = {}
        if (config.ipac_controllers) {
            for (const controller of config.ipac_controllers) {
                if (controller.button_labels) {
                    Object.assign(mergedLabels, controller.button_labels)
                }
            }
        }
        setButtonLabels(mergedLabels)

        // Set controller count and names
        controllerCount.value = config.ipac_controllers?.length || 1
        controllerNames.value = config.ipac_controllers?.map(c => c.device || '') || []

        // Extract configured buttons per controller
        const buttonsPerController: string[][] = []
        if (config.ipac_controllers) {
            for (const controller of config.ipac_controllers) {
                const buttons = controller.pin_mappings ? Object.keys(controller.pin_mappings) : []
                buttonsPerController.push(buttons)
            }
        }
        configuredButtons.value = buttonsPerController
    } catch (e) {
        // Silently fail - config is optional
        console.warn('Failed to load config data')
    }
}

onMounted(() => {
    loadAnimationList()
    loadConfigData()
    window.addEventListener('beforeunload', handleBeforeUnload)
})

onBeforeUnmount(() => {
    window.removeEventListener('beforeunload', handleBeforeUnload)
})
</script>
