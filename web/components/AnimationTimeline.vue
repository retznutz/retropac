<template>
    <div class="card timeline-card">
        <div class="card-header">
            <span class="card-title">Timeline</span>
            <div class="timeline-controls">
                <span v-if="selectedIndices.length > 1" class="selection-info">
                    {{ selectedIndices.length }} frames selected
                </span>
                <button v-if="selectedIndices.length > 1" class="btn btn-secondary btn-sm" @click="$emit('clear-selection')" title="Clear selection">
                    <i class="bx bx-x"></i> Clear
                </button>
                <div class="timeline-zoom">
                    <button class="btn btn-secondary btn-sm" @click="zoomOut" :disabled="zoom <= 0.4" title="Zoom out">
                        <i class="bx bx-minus"></i>
                    </button>
                    <button class="btn btn-secondary btn-sm" @click="zoomIn" :disabled="zoom >= 1" title="Zoom in">
                        <i class="bx bx-plus"></i>
                    </button>
                </div>
            </div>
        </div>
        <p class="timeline-hint">Ctrl/Cmd+click to select multiple frames. Shift+click to select range.</p>
        <div class="timeline">
            <div class="frame-container">
                <FrameCard v-for="(frame, index) in frames" :key="index" :frame="frame" :frame-number="index + 1"
                    :is-active="selectedIndex === index" 
                    :is-selected="selectedIndices.includes(index)"
                    :is-playing="isPlaying && playingIndex === index"
                    :is-dragging="dragIndex === index" :is-drag-over="dragOverIndex === index && dragIndex !== index"
                    :zoom="zoom" @select="onFrameClick($event, index)" @duplicate="$emit('duplicate-frame', index)"
                    @remove="$emit('remove-frame', index)" @dragstart="onDragStart($event, index)" @dragend="onDragEnd"
                    @dragover="onDragOver($event, index)" @dragleave="onDragLeave" @drop="onDrop(index)" />
                <div class="add-frame-btn" @click="$emit('add-frame')">
                    <i class="bx bx-plus"></i>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import type { AnimationFrame } from '~/types'

interface Props {
    frames: AnimationFrame[]
    selectedIndex: number
    selectedIndices: number[]
    isPlaying?: boolean
    playingIndex?: number
}

const props = withDefaults(defineProps<Props>(), {
    isPlaying: false,
    playingIndex: 0
})

const emit = defineEmits<{
    'select-frame': [index: number, event: { ctrlKey: boolean, shiftKey: boolean, metaKey: boolean }]
    'duplicate-frame': [index: number]
    'remove-frame': [index: number]
    'add-frame': []
    'reorder-frames': [fromIndex: number, toIndex: number]
    'clear-selection': []
}>()

// Timeline zoom (1 = full size, 0.4 = minimum)
const zoom = ref(1)

// Drag and drop state
const dragIndex = ref<number | null>(null)
const dragOverIndex = ref<number | null>(null)

function zoomIn() {
    zoom.value = Math.min(1, zoom.value + 0.2)
}

function zoomOut() {
    zoom.value = Math.max(0.4, zoom.value - 0.2)
}

function onDragStart(event: DragEvent, index: number) {
    dragIndex.value = index
    if (event.dataTransfer) {
        event.dataTransfer.effectAllowed = 'move'
        event.dataTransfer.setData('text/plain', index.toString())
    }
}

function onDragEnd() {
    dragIndex.value = null
    dragOverIndex.value = null
}

function onDragOver(event: DragEvent, index: number) {
    if (dragIndex.value !== null && dragIndex.value !== index) {
        dragOverIndex.value = index
    }
}

function onDragLeave() {
    dragOverIndex.value = null
}

function onFrameClick(event: MouseEvent, index: number) {
    emit('select-frame', index, {
        ctrlKey: event.ctrlKey,
        shiftKey: event.shiftKey,
        metaKey: event.metaKey
    })
}

function onDrop(targetIndex: number) {
    if (dragIndex.value === null || dragIndex.value === targetIndex) {
        dragIndex.value = null
        dragOverIndex.value = null
        return
    }

    emit('reorder-frames', dragIndex.value, targetIndex)

    dragIndex.value = null
    dragOverIndex.value = null
}
</script>

<style scoped>
.timeline-card {
    margin-top: 1rem;
    overflow: hidden;
}

.timeline-controls {
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.selection-info {
    font-size: 0.85rem;
    color: var(--accent);
    font-weight: 500;
}

.timeline-hint {
    font-size: 0.75rem;
    color: var(--text-secondary);
    margin: 0.25rem 1rem 0.5rem;
    opacity: 0.8;
}
</style>
