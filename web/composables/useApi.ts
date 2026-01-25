/**
 * API composable for RetroPac Animation Editor
 */

import type {
  Animation,
  AnimationListResponse,
  ButtonsResponse,
  SaveResponse,
  DeleteResponse,
  PlayResponse,
  StopResponse,
  SetAttractResponse,
  DuplicateResponse,
  RenameResponse
} from '~/types'

export function useApi() {
  const config = useRuntimeConfig()
  const baseUrl = config.public.apiBase || '/api'

  async function fetchJson<T>(url: string, options: RequestInit = {}): Promise<T> {
    const response = await fetch(`${baseUrl}${url}`, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      }
    })

    if (!response.ok) {
      const error = await response.json().catch(() => ({ error: 'Unknown error' }))
      throw new Error(error.error || `HTTP ${response.status}`)
    }

    return response.json()
  }

  return {
    /**
     * Get list of all animation names
     */
    async getAnimations(): Promise<AnimationListResponse> {
      return fetchJson<AnimationListResponse>('/animations')
    },

    /**
     * Get a specific animation by name
     */
    async getAnimation(name: string): Promise<Animation> {
      return fetchJson<Animation>(`/animations/${encodeURIComponent(name)}`)
    },

    /**
     * Save/update an animation
     */
    async saveAnimation(name: string, animation: Animation): Promise<SaveResponse> {
      return fetchJson<SaveResponse>(`/animations/${encodeURIComponent(name)}`, {
        method: 'PUT',
        body: JSON.stringify(animation)
      })
    },

    /**
     * Delete an animation
     */
    async deleteAnimation(name: string): Promise<DeleteResponse> {
      return fetchJson<DeleteResponse>(`/animations/${encodeURIComponent(name)}`, {
        method: 'DELETE'
      })
    },

    /**
     * Get list of valid button names
     */
    async getButtons(): Promise<ButtonsResponse> {
      return fetchJson<ButtonsResponse>('/buttons')
    },

    /**
     * Play animation on hardware
     */
    async playAnimation(name: string): Promise<PlayResponse> {
      return fetchJson<PlayResponse>(`/animations/${encodeURIComponent(name)}/play`, {
        method: 'POST'
      })
    },

    /**
     * Stop any running animation on hardware
     */
    async stopAnimation(): Promise<StopResponse> {
      return fetchJson<StopResponse>('/animations/stop', {
        method: 'POST'
      })
    },

    /**
     * Set animation as attract mode (updates RetroPie shell scripts)
     */
    async setAttractMode(name: string): Promise<SetAttractResponse> {
      return fetchJson<SetAttractResponse>(`/animations/${encodeURIComponent(name)}/set-attract`, {
        method: 'POST'
      })
    },

    /**
     * Duplicate an animation with a timestamp suffix
     */
    async duplicateAnimation(name: string): Promise<DuplicateResponse> {
      return fetchJson<DuplicateResponse>(`/animations/${encodeURIComponent(name)}/duplicate`, {
        method: 'POST'
      })
    },

    /**
     * Rename an animation file
     */
    async renameAnimation(oldName: string, newFilename: string): Promise<RenameResponse> {
      return fetchJson<RenameResponse>(`/animations/${encodeURIComponent(oldName)}/rename`, {
        method: 'POST',
        body: JSON.stringify({ newFilename })
      })
    }
  }
}
