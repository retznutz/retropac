<template>
  <div class="arcade-panel">
    <!-- Player 1 Section -->
    <div class="player-section" v-if="hasPlayerControls(1)">
      <div class="player-label">Player 1</div>

      <!-- Start/Coin Row -->
      <div class="control-row" v-if="isConfigured('P1_COIN') || isConfigured('P1_START')">
        <div v-if="isConfigured('P1_COIN')" class="arcade-button coin-btn" :class="{ selected: isSelected('P1_COIN') }"
          :style="getButtonStyle('P1_COIN')" @click="$emit('button-click', 'P1_COIN')" :title="getTooltip('P1_COIN')">C</div>
        <div v-if="isConfigured('P1_START')" class="arcade-button start-btn" :class="{ selected: isSelected('P1_START') }"
          :style="getButtonStyle('P1_START')" @click="$emit('button-click', 'P1_START')" :title="getTooltip('P1_START')">S</div>
      </div>

      <!-- Joystick -->
      <div v-if="isConfigured('P1_JOYSTICK')" class="joystick arcade-button" :class="{ selected: isSelected('P1_JOYSTICK') }"
        :style="getButtonStyle('P1_JOYSTICK')" @click="$emit('button-click', 'P1_JOYSTICK')" :title="getTooltip('P1_JOYSTICK')"></div>

      <!-- Action Buttons -->
      <div class="button-group" v-if="getConfiguredButtonCount(1) > 0" :style="getButtonGridStyle(1)">
        <div v-for="i in getConfiguredButtonCount(1)" :key="'P1_BUTTON' + i" class="arcade-button"
          :class="{ selected: isSelected('P1_BUTTON' + i) }" :style="getButtonStyle('P1_BUTTON' + i)"
          @click="$emit('button-click', 'P1_BUTTON' + i)" :title="getTooltip('P1_BUTTON' + i)">{{ i }}</div>
      </div>
    </div>

    <!-- Player 2 Section -->
    <div class="player-section" v-if="hasPlayerControls(2)">
      <div class="player-label">Player 2</div>

      <!-- Start/Coin Row -->
      <div class="control-row" v-if="isConfigured('P2_COIN') || isConfigured('P2_START')">
        <div v-if="isConfigured('P2_COIN')" class="arcade-button coin-btn" :class="{ selected: isSelected('P2_COIN') }"
          :style="getButtonStyle('P2_COIN')" @click="$emit('button-click', 'P2_COIN')" :title="getTooltip('P2_COIN')">C</div>
        <div v-if="isConfigured('P2_START')" class="arcade-button start-btn" :class="{ selected: isSelected('P2_START') }"
          :style="getButtonStyle('P2_START')" @click="$emit('button-click', 'P2_START')" :title="getTooltip('P2_START')">S</div>
      </div>

      <!-- Joystick -->
      <div v-if="isConfigured('P2_JOYSTICK')" class="joystick arcade-button" :class="{ selected: isSelected('P2_JOYSTICK') }"
        :style="getButtonStyle('P2_JOYSTICK')" @click="$emit('button-click', 'P2_JOYSTICK')" :title="getTooltip('P2_JOYSTICK')"></div>

      <!-- Action Buttons -->
      <div class="button-group" v-if="getConfiguredButtonCount(2) > 0" :style="getButtonGridStyle(2)">
        <div v-for="i in getConfiguredButtonCount(2)" :key="'P2_BUTTON' + i" class="arcade-button"
          :class="{ selected: isSelected('P2_BUTTON' + i) }" :style="getButtonStyle('P2_BUTTON' + i)"
          @click="$emit('button-click', 'P2_BUTTON' + i)" :title="getTooltip('P2_BUTTON' + i)">{{ i }}</div>
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
          <div v-if="isConfigured('P3_COIN')" class="arcade-button coin-btn" :class="{ selected: isSelected('P3_COIN') }"
            :style="getButtonStyle('P3_COIN')" @click="$emit('button-click', 'P3_COIN')" :title="getTooltip('P3_COIN')">C</div>
          <div v-if="isConfigured('P3_START')" class="arcade-button start-btn" :class="{ selected: isSelected('P3_START') }"
            :style="getButtonStyle('P3_START')" @click="$emit('button-click', 'P3_START')" :title="getTooltip('P3_START')">S</div>
        </div>
        <div v-if="isConfigured('P3_JOYSTICK')" class="joystick arcade-button" :class="{ selected: isSelected('P3_JOYSTICK') }"
          :style="getButtonStyle('P3_JOYSTICK')" @click="$emit('button-click', 'P3_JOYSTICK')" :title="getTooltip('P3_JOYSTICK')"></div>
        <div class="button-group" v-if="getConfiguredButtonCount(3) > 0" :style="getButtonGridStyle(3)">
          <div v-for="i in getConfiguredButtonCount(3)" :key="'P3_BUTTON' + i" class="arcade-button"
            :class="{ selected: isSelected('P3_BUTTON' + i) }" :style="getButtonStyle('P3_BUTTON' + i)"
            @click="$emit('button-click', 'P3_BUTTON' + i)" :title="getTooltip('P3_BUTTON' + i)">{{ i }}</div>
        </div>
      </div>

      <!-- Player 4 -->
      <div class="player-section" v-if="hasPlayerControls(4)">
        <div class="player-label">Player 4</div>
        <div class="control-row" v-if="isConfigured('P4_COIN') || isConfigured('P4_START')">
          <div v-if="isConfigured('P4_COIN')" class="arcade-button coin-btn" :class="{ selected: isSelected('P4_COIN') }"
            :style="getButtonStyle('P4_COIN')" @click="$emit('button-click', 'P4_COIN')" :title="getTooltip('P4_COIN')">C</div>
          <div v-if="isConfigured('P4_START')" class="arcade-button start-btn" :class="{ selected: isSelected('P4_START') }"
            :style="getButtonStyle('P4_START')" @click="$emit('button-click', 'P4_START')" :title="getTooltip('P4_START')">S</div>
        </div>
        <div v-if="isConfigured('P4_JOYSTICK')" class="joystick arcade-button" :class="{ selected: isSelected('P4_JOYSTICK') }"
          :style="getButtonStyle('P4_JOYSTICK')" @click="$emit('button-click', 'P4_JOYSTICK')" :title="getTooltip('P4_JOYSTICK')"></div>
        <div class="button-group" v-if="getConfiguredButtonCount(4) > 0" :style="getButtonGridStyle(4)">
          <div v-for="i in getConfiguredButtonCount(4)" :key="'P4_BUTTON' + i" class="arcade-button"
            :class="{ selected: isSelected('P4_BUTTON' + i) }" :style="getButtonStyle('P4_BUTTON' + i)"
            @click="$emit('button-click', 'P4_BUTTON' + i)" :title="getTooltip('P4_BUTTON' + i)">{{ i }}</div>
        </div>
      </div>

      <!-- Trackballs -->
      <div class="player-section" v-if="hasAnyTrackballs()">
        <div class="player-label">Trackballs</div>
        <div class="control-row">
          <template v-for="i in 4" :key="'P' + i + '_TRACKBALL'">
            <div v-if="isConfigured('P' + i + '_TRACKBALL')" class="arcade-button"
              :class="{ selected: isSelected('P' + i + '_TRACKBALL') }" :style="getButtonStyle('P' + i + '_TRACKBALL')"
              @click="$emit('button-click', 'P' + i + '_TRACKBALL')" :title="getTooltip('P' + i + '_TRACKBALL')">T{{ i }}</div>
          </template>
        </div>
      </div>
    </div>
  </details>
</template>

<script setup lang="ts">
import type { ButtonColorPair } from '~/types'
import { getButtonLabel } from '~/composables/useButtonLabels'

const props = defineProps<{
  buttons: ButtonColorPair[]
  selectedButtons: string[]
  configuredButtons?: string[]
}>()

// Helper to check if a button is configured in pin_mappings
function isConfigured(button: string): boolean {
  // If no configuredButtons provided, show all (backwards compatibility)
  if (!props.configuredButtons || props.configuredButtons.length === 0) {
    return true
  }
  return props.configuredButtons.includes(button)
}

// Compute how many action buttons are configured for each player
function getConfiguredButtonCount(player: number): number {
  if (!props.configuredButtons || props.configuredButtons.length === 0) {
    return 8 // Default to 8 if no config
  }
  let count = 0
  for (let i = 1; i <= 8; i++) {
    if (props.configuredButtons.includes(`P${player}_BUTTON${i}`)) {
      count = i // Track highest configured button number
    }
  }
  return count
}

// Get the number of columns for the button grid based on count
// Layout: 2 buttons = 2 cols, 4 = 2 cols, 6 = 3 cols, 8 = 4 cols
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
  if (!props.configuredButtons || props.configuredButtons.length === 0) {
    return true
  }
  const prefix = `P${player}_`
  return props.configuredButtons.some(btn => btn.startsWith(prefix))
}

// Check if any trackballs are configured
function hasAnyTrackballs(): boolean {
  if (!props.configuredButtons || props.configuredButtons.length === 0) {
    return true
  }
  return props.configuredButtons.some(btn => btn.includes('_TRACKBALL'))
}

defineEmits<{
  (e: 'button-click', button: string): void
}>()

function isSelected(button: string): boolean {
  return props.selectedButtons.includes(button)
}

function getButtonStyle(button: string): Record<string, string> {
  const btn = props.buttons.find(b => b.button === button)
  if (btn && btn.color && btn.color !== '#000000') {
    return {
      backgroundColor: btn.color,
      boxShadow: `0 0 15px ${btn.color}`
    }
  }
  return { backgroundColor: '#333' }
}

function getTooltip(buttonId: string): string {
  return getButtonLabel(buttonId, true)
}
</script>
