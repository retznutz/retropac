<template>
    <div class="frame-card" :class="{
        active: isActive,
        selected: isSelected,
        playing: isPlaying,
        dragging: isDragging,
        'drag-over': isDragOver,
        compact: isCompact
    }" :style="{ minWidth: minWidth + 'px' }" draggable="true" @dragstart="$emit('dragstart', $event)"
        @dragend="$emit('dragend')" @dragover.prevent="$emit('dragover', $event)" @dragleave="$emit('dragleave')"
        @drop.prevent="$emit('drop')" @click="$emit('select', $event)">
        <div class="frame-header">
            <span class="frame-number">{{ showFullLabel ? 'Frame ' : '' }}{{ frameNumber }}</span>
            <div class="frame-actions" v-if="showActions">
                <button class="btn btn-secondary btn-sm" @click.stop="$emit('duplicate')" title="Duplicate frame">
                    <i class="bx bx-copy"></i>
                </button>
                <button class="btn btn-danger btn-sm" @click.stop="$emit('remove')" title="Delete frame">
                    <i class="bx bx-x"></i>
                </button>
            </div>
        </div>
        <div class="frame-buttons">
            <div v-for="(btn, index) in frame.buttons" :key="index" class="frame-button-preview"
                :style="{ backgroundColor: btn.color }" :title="btn.button"></div>
        </div>
        <div v-if="frame.fade" class="frame-fade-info">
            Fade: {{ frame.fade_speed_ms }}ms
        </div>
    </div>
</template>

<script setup lang="ts">
import type { AnimationFrame } from '~/types'

interface Props {
    frame: AnimationFrame
    frameNumber: number
    isActive?: boolean
    isSelected?: boolean
    isPlaying?: boolean
    isDragging?: boolean
    isDragOver?: boolean
    zoom?: number
}

const props = withDefaults(defineProps<Props>(), {
    isActive: false,
    isSelected: false,
    isPlaying: false,
    isDragging: false,
    isDragOver: false,
    zoom: 1
})

defineEmits<{
    select: [event: MouseEvent]
    duplicate: []
    remove: []
    dragstart: [event: DragEvent]
    dragend: []
    dragover: [event: DragEvent]
    dragleave: []
    drop: []
}>()

const minWidth = computed(() => 200 * props.zoom)
const isCompact = computed(() => props.zoom < 0.7)
const showFullLabel = computed(() => props.zoom >= 0.7)
const showActions = computed(() => props.zoom >= 0.6)
</script>

<style scoped>
.frame-fade-info {
    font-size: 0.75rem;
    color: var(--text-secondary);
    margin-top: 0.5rem;
}
</style>
