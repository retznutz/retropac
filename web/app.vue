<template>
  <div>
    <header class="header">
      <a href="https://github.com/retznutz/retropac" target="_blank" rel="noopener">
        <img src="~/assets/logo.svg" alt="RetroPac Animation Editor" class="header-logo" />
      </a>
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
            <button class="btn btn-primary btn-sm" @click="createNewAnimation"><i class="bx bx-plus"></i> New</button>
          </div>
          <div class="animation-list">
            <div v-for="anim in animations" :key="anim.filename" class="animation-item"
              :class="{ active: currentAnimationName === anim.filename }" @click="loadAnimation(anim.filename)">
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
          </div>

          <div v-if="currentAnimation">
            <!-- Arcade Panel Preview -->
            <ArcadePanel :buttons="isPlaying ? previewButtons : (selectedFrame?.buttons || [])"
              :selected-buttons="isPlaying ? [] : selectedButtons" @button-click="toggleButton" />
            <div v-if="isPlaying" class="preview-indicator">
              <i class="bx bx-radio-circle-marked bx-flashing"></i> Playing Frame {{ previewFrameIndex + 1 }} / {{
                currentAnimation.frames.length }}
            </div>

            <!-- Timeline -->
            <AnimationTimeline
              :frames="currentAnimation.frames"
              :selected-index="selectedFrameIndex"
              :is-playing="isPlaying"
              :playing-index="previewFrameIndex"
              @select-frame="selectFrame"
              @duplicate-frame="duplicateFrame"
              @remove-frame="removeFrame"
              @add-frame="addFrame"
              @reorder-frames="reorderFrames"
            />
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
            </div>
            <div class="form-group">
              <label>Name</label>
              <input type="text" class="form-control" v-model="currentAnimation.name" />
            </div>
            <div class="form-group">
              <label>File Name</label>
              <div class="filename-input-group">
                <input type="text" class="form-control" v-model="editableFilename" @blur="renameAnimation"
                  @keyup.enter="renameAnimation" />
              </div>
            </div>
            <div class="form-group">
              <label>Speed (ms)</label>
              <input type="number" class="form-control" v-model.number="currentAnimation.speed" min="10" />
            </div>
            <div class="form-group">
              <div class="form-check">
                <input type="checkbox" id="loop" v-model="currentAnimation.loop" />
                <label for="loop">Loop Animation</label>
              </div>
            </div>
          </div>

          <!-- Frame Properties -->
          <div class="card" v-if="selectedFrame">
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
              <input type="number" class="form-control" v-model.number="selectedFrame.fade_speed_ms" min="0" />
            </div>
          </div>

          <!-- Button Color -->
          <div class="card" v-if="selectedButtons.length > 0">
            <div class="card-header">
              <span class="card-title">Button Color</span>
            </div>
            <p style="font-size: 0.85rem; color: var(--text-secondary); margin-bottom: 0.5rem;">
              Selected: {{ selectedButtons.join(', ') }}
            </p>
            <div class="color-picker-wrapper">
              <div class="color-preview" :style="{ backgroundColor: selectedColor }"></div>
              <input type="color" v-model="selectedColor" @input="applyColor" />
            </div>
            <div class="btn-group" style="margin-top: 0.5rem;">
              <button class="btn btn-secondary btn-sm" @click="applyColor">Apply</button>
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
          <strong>RetroPac</strong> — An LED animation editor for Ultimarc I-PAC controllers. Create, preview, and
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
import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue'
import { useApi } from '~/composables/useApi'
import type { ButtonColorPair, AnimationFrame, AnimationListItem, Animation } from '~/types'

const api = useApi()

const animations = ref<AnimationListItem[]>([])
const currentAnimationName = ref<string | null>(null)
const currentAnimation = ref<Animation | null>(null)
const selectedFrameIndex = ref(0)
const selectedButtons = ref<string[]>([])
const selectedColor = ref('#FF0000')
const editableFilename = ref('')

// Playback preview state
const isPlaying = ref(false)
const isPlayingOnHardware = ref(false)
const previewFrameIndex = ref(0)
const previewButtons = ref<ButtonColorPair[]>([])
let playbackTimeout: ReturnType<typeof setTimeout> | null = null
let fadeStartTime = 0
let fadeAnimationFrame: number | null = null

const toast = ref({ show: false, message: '', type: 'success' })

// Dirty state tracking
const isDirty = ref(false)
let savedSnapshot = ''

const selectedFrame = computed(() => {
  if (!currentAnimation.value || selectedFrameIndex.value < 0) return null
  return currentAnimation.value.frames[selectedFrameIndex.value]
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
    currentAnimation.value = data
    currentAnimationName.value = name
    editableFilename.value = name
    selectedFrameIndex.value = 0
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
function selectFrame(index: number) {
  selectedFrameIndex.value = index
  selectedButtons.value = []
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
}

// Button selection and color
function toggleButton(button: string) {
  const idx = selectedButtons.value.indexOf(button)
  if (idx >= 0) {
    selectedButtons.value.splice(idx, 1)
  } else {
    selectedButtons.value.push(button)

    // Get current color if button already has one
    const frame = selectedFrame.value
    if (frame) {
      const existing = frame.buttons.find(b => b.button === button)
      if (existing) {
        selectedColor.value = existing.color
      }
    }
  }
}

function applyColor() {
  if (!selectedFrame.value || selectedButtons.value.length === 0) return

  for (const button of selectedButtons.value) {
    const existing = selectedFrame.value.buttons.find(b => b.button === button)
    if (existing) {
      existing.color = selectedColor.value
    } else {
      selectedFrame.value.buttons.push({
        button,
        color: selectedColor.value
      })
    }
  }
}

function removeSelectedButtons() {
  if (!selectedFrame.value) return

  selectedFrame.value.buttons = selectedFrame.value.buttons.filter(
    b => !selectedButtons.value.includes(b.button)
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

    // Get list of buttons in this frame
    const frameButtonNames = new Set(frame.buttons.map(b => b.button))

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
    const frameButtonNames = new Set(frame.buttons.map(b => b.button))
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
  const fromRgb = hexToRgb(from)
  const toRgb = hexToRgb(to)
  const r = Math.round(fromRgb.r + (toRgb.r - fromRgb.r) * t)
  const g = Math.round(fromRgb.g + (toRgb.g - fromRgb.g) * t)
  const b = Math.round(fromRgb.b + (toRgb.b - fromRgb.b) * t)
  return rgbToHex(r, g, b)
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

onMounted(() => {
  loadAnimationList()
  window.addEventListener('beforeunload', handleBeforeUnload)
})

onBeforeUnmount(() => {
  window.removeEventListener('beforeunload', handleBeforeUnload)
})
</script>
