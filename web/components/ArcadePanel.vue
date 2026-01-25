<template>
  <div class="arcade-panel">
    <!-- Player 1 Section -->
    <div class="player-section">
      <div class="player-label">Player 1</div>

      <!-- Start/Coin Row -->
      <div class="control-row">
        <div class="arcade-button coin-btn" :class="{ selected: isSelected('P1_COIN') }"
          :style="getButtonStyle('P1_COIN')" @click="$emit('button-click', 'P1_COIN')" title="P1 Coin">C</div>
        <div class="arcade-button start-btn" :class="{ selected: isSelected('P1_START') }"
          :style="getButtonStyle('P1_START')" @click="$emit('button-click', 'P1_START')" title="P1 Start">S</div>
      </div>

      <!-- Joystick -->
      <div class="joystick arcade-button" :class="{ selected: isSelected('P1_JOYSTICK') }"
        :style="getButtonStyle('P1_JOYSTICK')" @click="$emit('button-click', 'P1_JOYSTICK')" title="P1 Joystick"></div>

      <!-- Action Buttons -->
      <div class="button-group">
        <div v-for="i in 8" :key="'P1_BUTTON' + i" class="arcade-button"
          :class="{ selected: isSelected('P1_BUTTON' + i) }" :style="getButtonStyle('P1_BUTTON' + i)"
          @click="$emit('button-click', 'P1_BUTTON' + i)" :title="'P1 Button ' + i">{{ i }}</div>
      </div>
    </div>

    <!-- Player 2 Section -->
    <div class="player-section">
      <div class="player-label">Player 2</div>

      <!-- Start/Coin Row -->
      <div class="control-row">
        <div class="arcade-button coin-btn" :class="{ selected: isSelected('P2_COIN') }"
          :style="getButtonStyle('P2_COIN')" @click="$emit('button-click', 'P2_COIN')" title="P2 Coin">C</div>
        <div class="arcade-button start-btn" :class="{ selected: isSelected('P2_START') }"
          :style="getButtonStyle('P2_START')" @click="$emit('button-click', 'P2_START')" title="P2 Start">S</div>
      </div>

      <!-- Joystick -->
      <div class="joystick arcade-button" :class="{ selected: isSelected('P2_JOYSTICK') }"
        :style="getButtonStyle('P2_JOYSTICK')" @click="$emit('button-click', 'P2_JOYSTICK')" title="P2 Joystick"></div>

      <!-- Action Buttons -->
      <div class="button-group">
        <div v-for="i in 8" :key="'P2_BUTTON' + i" class="arcade-button"
          :class="{ selected: isSelected('P2_BUTTON' + i) }" :style="getButtonStyle('P2_BUTTON' + i)"
          @click="$emit('button-click', 'P2_BUTTON' + i)" :title="'P2 Button ' + i">{{ i }}</div>
      </div>
    </div>
  </div>

  <!-- Additional Controls (Players 3-4, Trackballs) - Collapsible -->
  <details style="margin-top: 1rem;">
    <summary style="cursor: pointer; color: var(--text-secondary);">
      More Controls (Players 3-4, Trackballs)
    </summary>
    <div class="arcade-panel" style="margin-top: 0.5rem;">
      <!-- Player 3 -->
      <div class="player-section">
        <div class="player-label">Player 3</div>
        <div class="control-row">
          <div class="arcade-button coin-btn" :class="{ selected: isSelected('P3_COIN') }"
            :style="getButtonStyle('P3_COIN')" @click="$emit('button-click', 'P3_COIN')">C</div>
          <div class="arcade-button start-btn" :class="{ selected: isSelected('P3_START') }"
            :style="getButtonStyle('P3_START')" @click="$emit('button-click', 'P3_START')">S</div>
        </div>
        <div class="joystick arcade-button" :class="{ selected: isSelected('P3_JOYSTICK') }"
          :style="getButtonStyle('P3_JOYSTICK')" @click="$emit('button-click', 'P3_JOYSTICK')"></div>
        <div class="button-group">
          <div v-for="i in 8" :key="'P3_BUTTON' + i" class="arcade-button"
            :class="{ selected: isSelected('P3_BUTTON' + i) }" :style="getButtonStyle('P3_BUTTON' + i)"
            @click="$emit('button-click', 'P3_BUTTON' + i)">{{ i }}</div>
        </div>
      </div>

      <!-- Player 4 -->
      <div class="player-section">
        <div class="player-label">Player 4</div>
        <div class="control-row">
          <div class="arcade-button coin-btn" :class="{ selected: isSelected('P4_COIN') }"
            :style="getButtonStyle('P4_COIN')" @click="$emit('button-click', 'P4_COIN')">C</div>
          <div class="arcade-button start-btn" :class="{ selected: isSelected('P4_START') }"
            :style="getButtonStyle('P4_START')" @click="$emit('button-click', 'P4_START')">S</div>
        </div>
        <div class="joystick arcade-button" :class="{ selected: isSelected('P4_JOYSTICK') }"
          :style="getButtonStyle('P4_JOYSTICK')" @click="$emit('button-click', 'P4_JOYSTICK')"></div>
        <div class="button-group">
          <div v-for="i in 8" :key="'P4_BUTTON' + i" class="arcade-button"
            :class="{ selected: isSelected('P4_BUTTON' + i) }" :style="getButtonStyle('P4_BUTTON' + i)"
            @click="$emit('button-click', 'P4_BUTTON' + i)">{{ i }}</div>
        </div>
      </div>

      <!-- Trackballs -->
      <div class="player-section">
        <div class="player-label">Trackballs</div>
        <div class="control-row">
          <div v-for="i in 4" :key="'P' + i + '_TRACKBALL'" class="arcade-button"
            :class="{ selected: isSelected('P' + i + '_TRACKBALL') }" :style="getButtonStyle('P' + i + '_TRACKBALL')"
            @click="$emit('button-click', 'P' + i + '_TRACKBALL')" :title="'P' + i + ' Trackball'">T{{ i }}</div>
        </div>
      </div>
    </div>
  </details>
</template>

<script setup lang="ts">
import type { ButtonColorPair } from '~/types'

const props = defineProps<{
  buttons: ButtonColorPair[]
  selectedButtons: string[]
}>()

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
</script>
