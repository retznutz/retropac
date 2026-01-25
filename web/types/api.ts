/**
 * API response type definitions
 */

import type { AnimationListItem } from './animation'

export interface AnimationListResponse {
    animations: AnimationListItem[]
}

export interface ButtonsResponse {
    buttons: string[]
}

export interface SaveResponse {
    success: boolean
    name: string
}

export interface DeleteResponse {
    success: boolean
    deleted: string
}

export interface PlayResponse {
    success: boolean
    animation?: string
    message?: string
    error?: string
}

export interface StopResponse {
    success: boolean
    message?: string
    error?: string
}

export interface SetAttractResponse {
    success: boolean
    animation?: string
    message?: string
    error?: string
}

export interface DuplicateResponse {
    success: boolean
    original: string
    duplicate: string
    error?: string
}

export interface RenameResponse {
    success: boolean
    oldFilename: string
    newFilename: string
    error?: string
}
