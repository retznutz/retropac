/**
 * API composable for RetroPac Animation Editor
 */

interface AnimationListItem {
  filename: string
  name: string
}

interface AnimationListResponse {
  animations: AnimationListItem[]
}

interface Animation {
  name: string
  speed: number
  loop: boolean
  frames: AnimationFrame[]
}

interface AnimationFrame {
  buttons: ButtonColorPair[]
  fade: boolean
  fade_speed_ms: number
}

interface ButtonColorPair {
  button: string
  color: string
}

interface ButtonsResponse {
  buttons: string[]
}

interface SaveResponse {
  success: boolean
  name: string
}

interface DeleteResponse {
  success: boolean
  deleted: string
}

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
    }
  }
}
