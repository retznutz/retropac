#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "retropac.h"
#include "ultimarc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Gamma correction value for LEDs (2.2 is standard) */
#define LED_GAMMA 2.2f

/* Hardware fade configuration */
#define USE_HARDWARE_FADE 1    /* Set to 0 to disable hardware fade */
#define HW_FADE_RATE_FAST   3    /* Fast hardware fade (for quick animations) */
#define HW_FADE_RATE_MEDIUM 8    /* Medium hardware fade (good default) */
#define HW_FADE_RATE_SLOW   15   /* Slow hardware fade (for breathing effects) */
#define HW_FADE_RATE_MAX    15   /* Maximum rate - higher values cause dim LEDs */

/* Pre-computed gamma lookup table */
static uint8_t gamma_lut[256];
static int gamma_lut_initialized = 0;
static int hardware_fade_enabled = 0;

/* Initialize gamma lookup table */
static void init_gamma_lut(void) {
    if (gamma_lut_initialized) return;
    for (int i = 0; i < 256; i++) {
        gamma_lut[i] = (uint8_t)(powf(i / 255.0f, LED_GAMMA) * 255.0f + 0.5f);
    }
    gamma_lut_initialized = 1;
}

/* Apply gamma correction to a color */
RGBColor gamma_correct_color(RGBColor color) {
    init_gamma_lut();
    RGBColor result;
    result.r = gamma_lut[color.r];
    result.g = gamma_lut[color.g];
    result.b = gamma_lut[color.b];
    return result;
}

/* Easing functions for smooth transitions */

/* Ease-in-out (smooth start and end) - most natural looking */
float ease_in_out(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

/* Ease-in (starts slow, accelerates) */
float ease_in(float t) {
    return t * t;
}

/* Ease-out (starts fast, decelerates) */
float ease_out(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

/* Smooth-step (cubic ease-in-out, very smooth) */
float smooth_step(float t) {
    return t * t * (3.0f - 2.0f * t);
}

/* Smoother-step (quintic, even smoother than smooth-step) */
float smoother_step(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/* ========================================================================
 * Hardware Fade Integration
 * ======================================================================== */

/**
 * Enable hardware fade on all controllers
 * This makes LED transitions much smoother by offloading interpolation
 * to the hardware. The fade_rate determines transition speed:
 * - 0 = instant (no fade)
 * - 1-50 = fast
 * - 50-100 = medium
 * - 100-255 = slow
 */
void animation_enable_hardware_fade(IpacController *controllers, int count, uint8_t fade_rate) {
#if USE_HARDWARE_FADE
    if (!controllers || count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            /* Check if device supports hardware fade */
            UltimarcDeviceType type = ultimarc_detect_device_type(
                controllers[i].vendor_id, controllers[i].product_id);
            const UltimarcDeviceInfo *info = ultimarc_get_device_info(type);
            
            if (info->has_fade) {
                int result = pac_led_set_fade_time(&controllers[i], fade_rate);
                if (result >= 0) {
                    printf("Hardware fade enabled on %s (rate: %d)\n", 
                           controllers[i].device_name, fade_rate);
                    hardware_fade_enabled = 1;
                }
            }
        }
    }
#else
    (void)controllers;
    (void)count;
    (void)fade_rate;
#endif
}

/**
 * Disable hardware fade (instant transitions)
 */
void animation_disable_hardware_fade(IpacController *controllers, int count) {
#if USE_HARDWARE_FADE
    if (!controllers || count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        if (controllers[i].usb_handle) {
            UltimarcDeviceType type = ultimarc_detect_device_type(
                controllers[i].vendor_id, controllers[i].product_id);
            const UltimarcDeviceInfo *info = ultimarc_get_device_info(type);
            
            if (info->has_fade) {
                pac_led_set_fade_time(&controllers[i], 0);
            }
        }
    }
    hardware_fade_enabled = 0;
#else
    (void)controllers;
    (void)count;
#endif
}

/**
 * Check if hardware fade is currently enabled
 */
bool animation_is_hardware_fade_enabled(void) {
    return hardware_fade_enabled != 0;
}

/**
 * Get optimal hardware fade rate for animation type
 * Returns 0 if hardware fade should not be used
 */
static uint8_t get_fade_rate_for_animation(AnimationType type) {
    switch (type) {
        case ANIM_BREATHING:
            return HW_FADE_RATE_SLOW;   /* Slow fade for breathing */
        case ANIM_RAINBOW:
        case ANIM_COLOR_CYCLE:
            return HW_FADE_RATE_MEDIUM; /* Medium for smooth color transitions */
        case ANIM_CHASE:
            return HW_FADE_RATE_FAST;   /* Fast for chase tail effect */
        case ANIM_SPARKLE:
            return 0;  /* Sparkle needs instant on/off */
        case ANIM_STATIC:
        case ANIM_NONE:
        default:
            return HW_FADE_RATE_MEDIUM;
    }
}

/* Global flag for signal handling */
static volatile sig_atomic_t exit_requested = 0;

/* Signal handler for graceful shutdown */
static void signal_handler(int signum) {
    (void)signum;
    exit_requested = 1;
}

/* Setup signal handlers for SIGTERM and SIGINT */
void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

/* Check if exit has been requested */
bool should_exit(void) {
    return exit_requested != 0;
}

/* Convert animation type string to enum */
AnimationType animation_type_from_string(const char *name) {
    if (!name) return ANIM_NONE;
    
    if (strcasecmp(name, "rainbow") == 0) return ANIM_RAINBOW;
    if (strcasecmp(name, "breathing") == 0) return ANIM_BREATHING;
    if (strcasecmp(name, "chase") == 0) return ANIM_CHASE;
    if (strcasecmp(name, "sparkle") == 0) return ANIM_SPARKLE;
    if (strcasecmp(name, "color_cycle") == 0) return ANIM_COLOR_CYCLE;
    if (strcasecmp(name, "colorcycle") == 0) return ANIM_COLOR_CYCLE;
    if (strcasecmp(name, "static") == 0) return ANIM_STATIC;
    if (strcasecmp(name, "none") == 0) return ANIM_NONE;
    
    return ANIM_NONE;
}

/* Convert animation type enum to string */
const char *animation_type_to_string(AnimationType type) {
    switch (type) {
        case ANIM_RAINBOW:     return "rainbow";
        case ANIM_BREATHING:   return "breathing";
        case ANIM_CHASE:       return "chase";
        case ANIM_SPARKLE:     return "sparkle";
        case ANIM_COLOR_CYCLE: return "color_cycle";
        case ANIM_STATIC:      return "static";
        case ANIM_NONE:
        default:               return "none";
    }
}

/* HSV to RGB conversion for rainbow effect */
static RGBColor hsv_to_rgb(float h, float s, float v) {
    RGBColor rgb = {0, 0, 0};
    
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    rgb.r = (uint8_t)((r + m) * 255);
    rgb.g = (uint8_t)((g + m) * 255);
    rgb.b = (uint8_t)((b + m) * 255);
    
    return rgb;
}

/* Scale color by brightness factor (0.0 - 1.0) */
static RGBColor scale_color(RGBColor color, float brightness) {
    RGBColor result;
    result.r = (uint8_t)(color.r * brightness);
    result.g = (uint8_t)(color.g * brightness);
    result.b = (uint8_t)(color.b * brightness);
    return result;
}

/* Create animation state */
AnimationState *animation_create(AnimationConfig *config, IpacController *controllers, 
                                  int controller_count,
                                  ButtonConfig *initial_buttons, int button_count) {
    AnimationState *state = calloc(1, sizeof(AnimationState));
    if (!state) return NULL;
    
    state->config = config;
    state->controllers = controllers;
    state->controller_count = controller_count;
    state->running = false;
    state->frame = 0;
    
    /* Copy initial button states */
    state->total_buttons = button_count;
    state->button_states = calloc(button_count, sizeof(ButtonConfig));
    if (!state->button_states) {
        free(state);
        return NULL;
    }
    memcpy(state->button_states, initial_buttons, button_count * sizeof(ButtonConfig));
    
    return state;
}

/* Destroy animation state */
void animation_destroy(AnimationState *state) {
    if (state) {
        free(state->button_states);
        free(state);
    }
}

/* Stop animation */
void animation_stop(AnimationState *state) {
    if (state) {
        state->running = false;
    }
}

/* Rainbow animation frame */
static void animate_rainbow(AnimationState *state) {
    float hue_offset = (state->frame * 5) % 360;
    
    for (int i = 0; i < state->total_buttons; i++) {
        float hue = fmodf(hue_offset + (i * 30), 360);
        state->button_states[i].color = hsv_to_rgb(hue, 1.0f, 1.0f);
    }
}

/* Breathing/pulse animation frame - using smooth easing */
static void animate_breathing(AnimationState *state) {
    /* Calculate position in cycle (0.0 to 1.0) with 120 frame period */
    int cycle_length = 120;
    int frame_in_cycle = state->frame % cycle_length;
    float t;
    
    /* First half: fade up (0 to 0.5), second half: fade down (0.5 to 1.0) */
    if (frame_in_cycle < cycle_length / 2) {
        t = (float)frame_in_cycle / (cycle_length / 2);  /* 0.0 to 1.0 up */
    } else {
        t = 1.0f - ((float)(frame_in_cycle - cycle_length / 2) / (cycle_length / 2));  /* 1.0 to 0.0 down */
    }
    
    /* Apply smooth easing for more natural breathing feel */
    float brightness = smoother_step(t);
    
    /* Ensure minimum brightness so LEDs don't go completely off */
    brightness = 0.1f + (brightness * 0.9f);
    
    RGBColor base = state->config->base_color;
    RGBColor scaled = scale_color(base, brightness);
    
    for (int i = 0; i < state->total_buttons; i++) {
        state->button_states[i].color = scaled;
    }
}

/* Chase/running light animation frame - with smooth tail fade */
static void animate_chase(AnimationState *state) {
    int active_pos = state->frame % state->total_buttons;
    int tail_length = 4;  /* Slightly longer tail for smoother effect */
    
    RGBColor off = {0, 0, 0};
    RGBColor base = state->config->base_color;
    
    for (int i = 0; i < state->total_buttons; i++) {
        int distance = (active_pos - i + state->total_buttons) % state->total_buttons;
        
        if (distance == 0) {
            state->button_states[i].color = base;
        } else if (distance <= tail_length) {
            /* Apply ease-out for natural tail fade (bright to dim) */
            float t = (float)distance / (tail_length + 1);
            float fade = 1.0f - ease_out(t);
            state->button_states[i].color = scale_color(base, fade);
        } else {
            state->button_states[i].color = off;
        }
    }
}

/* Sparkle animation frame */
static void animate_sparkle(AnimationState *state) {
    RGBColor base = state->config->base_color;
    
    for (int i = 0; i < state->total_buttons; i++) {
        /* Random chance to sparkle (10% per frame) */
        if (rand() % 100 < 10) {
            state->button_states[i].color = base;
        } else {
            /* Fade existing color */
            RGBColor current = state->button_states[i].color;
            state->button_states[i].color = scale_color(current, 0.8f);
        }
    }
}

/* Color cycle animation frame */
static void animate_color_cycle(AnimationState *state) {
    if (state->config->color_count == 0 || !state->config->colors) {
        return;
    }
    
    int color_idx = (state->frame / 30) % state->config->color_count;
    RGBColor color = state->config->colors[color_idx];
    
    for (int i = 0; i < state->total_buttons; i++) {
        state->button_states[i].color = color;
    }
}

/* Execute single animation step */
void animation_step(AnimationState *state) {
    if (!state || !state->config) return;
    
    switch (state->config->type) {
        case ANIM_RAINBOW:
            animate_rainbow(state);
            break;
        case ANIM_BREATHING:
            animate_breathing(state);
            break;
        case ANIM_CHASE:
            animate_chase(state);
            break;
        case ANIM_SPARKLE:
            animate_sparkle(state);
            break;
        case ANIM_COLOR_CYCLE:
            animate_color_cycle(state);
            break;
        case ANIM_STATIC:
        case ANIM_NONE:
        default:
            /* No animation - keep current colors */
            return;
    }
    
    /* Send updated colors to hardware on all controllers */
    if (state->controllers && state->controller_count > 0) {
        for (int i = 0; i < state->total_buttons; i++) {
            ipac_set_led_all(state->controllers, state->controller_count,
                            state->button_states[i].button,
                            state->button_states[i].color);
        }
    }
    
    state->frame++;
}

/* Sleep for specified milliseconds */
static void sleep_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/* Run animation loop (blocking) */
void animation_run(AnimationState *state) {
    if (!state) return;
    
    /* Check if this is a custom animation or built-in */
    bool is_custom = (state->custom_anim != NULL);
    
    state->running = true;
    state->frame = 0;
    state->custom_frame_idx = 0;
    
    int speed = is_custom ? state->custom_anim->speed_ms : 
                (state->config ? state->config->speed_ms : 50);
    if (speed <= 0) speed = 50;  /* Default 50ms = 20fps */
    
    const char *anim_name = is_custom ? state->custom_anim->name : 
                            (state->config ? animation_type_to_string(state->config->type) : "unknown");
    
    printf("Starting %s animation (speed: %dms)\n", anim_name, speed);
    printf("Press Ctrl+C to stop\n");
    
    /* Enable hardware fade for smoother transitions */
    uint8_t fade_rate = 0;
    
    if (is_custom && state->custom_anim) {
        /* Use per-animation hardware fade settings */
        if (state->custom_anim->hardware_fade) {
            fade_rate = state->custom_anim->hardware_fade_rate;
            if (fade_rate == 0) fade_rate = HW_FADE_RATE_MEDIUM;
        }
    } else if (state->config) {
        fade_rate = get_fade_rate_for_animation(state->config->type);
    }
    
    if (fade_rate > 0) {
        animation_enable_hardware_fade(state->controllers, state->controller_count, fade_rate);
        printf("Hardware fade enabled (rate: %d) for smoother transitions\n", fade_rate);
    } else {
        /* Ensure fade is disabled (instant transitions) */
        animation_disable_hardware_fade(state->controllers, state->controller_count);
        printf("Hardware fade disabled (instant transitions)\n");
    }
    
    /* Seed random for sparkle effect */
    srand((unsigned int)time(NULL));
    
    while (state->running && !should_exit()) {
        if (is_custom) {
            animation_step_custom(state);
            
            /* Check if non-looping animation finished */
            if (!state->custom_anim->loop && 
                state->custom_frame_idx >= state->custom_anim->frame_count) {
                break;
            }
        } else {
            animation_step(state);
        }
        sleep_ms(speed);
    }
    
    /* Disable hardware fade when done */
    if (fade_rate > 0) {
        animation_disable_hardware_fade(state->controllers, state->controller_count);
    }
    
    state->running = false;
    printf("Animation stopped\n");
}

/* Create animation state for custom animation */
AnimationState *animation_create_custom(CustomAnimation *custom_anim, IpacController *controllers,
                                         int controller_count,
                                         ButtonConfig *initial_buttons, int button_count) {
    (void)initial_buttons;  /* Not used - we create all buttons */
    (void)button_count;     /* Not used - we use BUTTON_MAX */
    
    if (!custom_anim) return NULL;
    
    AnimationState *state = calloc(1, sizeof(AnimationState));
    if (!state) return NULL;
    
    state->config = NULL;  /* No built-in config */
    state->custom_anim = custom_anim;
    state->controllers = controllers;
    state->controller_count = controller_count;
    state->running = false;
    state->frame = 0;
    state->custom_frame_idx = 0;
    
    /* Create state for ALL buttons so we can turn any of them on/off */
    state->total_buttons = BUTTON_MAX;
    state->button_states = calloc(BUTTON_MAX, sizeof(ButtonConfig));
    if (!state->button_states) {
        free(state);
        return NULL;
    }
    
    /* Initialize all buttons with their button type and black color */
    RGBColor off = {0, 0, 0};
    for (int i = 0; i < BUTTON_MAX; i++) {
        state->button_states[i].button = (ButtonType)i;
        state->button_states[i].color = off;
        
        /* Send to hardware on all controllers */
        if (controllers && controller_count > 0) {
            ipac_set_led_all(controllers, controller_count, (ButtonType)i, off);
        }
    }
    
    return state;
}

/* Linear interpolation between colors with optional easing */
static RGBColor lerp_color(RGBColor from, RGBColor to, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    /* Apply smooth easing for more natural color transitions */
    t = smooth_step(t);
    
    RGBColor result;
    result.r = (uint8_t)(from.r + (to.r - from.r) * t);
    result.g = (uint8_t)(from.g + (to.g - from.g) * t);
    result.b = (uint8_t)(from.b + (to.b - from.b) * t);
    return result;
}

/* Execute single custom animation step */
void animation_step_custom(AnimationState *state) {
    if (!state || !state->custom_anim) return;
    
    CustomAnimation *anim = state->custom_anim;
    
    /* Check if we've reached the end */
    if (state->custom_frame_idx >= anim->frame_count) {
        if (anim->loop) {
            state->custom_frame_idx = 0;
            state->frame = 0;
        } else {
            return; /* Animation complete */
        }
    }
    
    CustomAnimationFrame *frame = &anim->frames[state->custom_frame_idx];
    
    /* Calculate elapsed time on current frame (state->frame counts step intervals) */
    int elapsed_ms = state->frame * anim->speed_ms;
    
    /* Determine if this frame uses fading */
    bool use_fade = frame->fade && frame->fade_speed_ms > 0;
    int fade_duration = use_fade ? frame->fade_speed_ms : 0;
    
    /*
     * Hardware fade optimization:
     * When hardware fade is enabled, we skip software interpolation and just
     * send the target colors directly. The hardware handles smooth transitions.
     * This is more efficient and produces smoother results.
     */
    bool use_hw_fade = hardware_fade_enabled && use_fade;
    
    /* Calculate fade progress (0.0 to 1.0) - only needed for software fade */
    float fade_progress = 1.0f;
    float eased_progress = 1.0f;
    if (use_fade && !use_hw_fade && fade_duration > 0) {
        fade_progress = (float)elapsed_ms / (float)fade_duration;
        if (fade_progress > 1.0f) fade_progress = 1.0f;
        /* Apply smooth easing to the fade */
        eased_progress = smooth_step(fade_progress);
    }
    
    /* Process all buttons - first handle buttons NOT in this frame (fade to black) */
    for (int i = 0; i < state->total_buttons; i++) {
        /* Check if this button is in the current frame */
        bool in_frame = false;
        for (int b = 0; b < frame->button_count; b++) {
            if (frame->buttons[b].button == (ButtonType)i) {
                in_frame = true;
                break;
            }
        }
        
        if (!in_frame) {
            /* Button not in frame - fade to black */
            RGBColor black = {0, 0, 0};
            RGBColor target = black;
            
            if (use_hw_fade) {
                /* Hardware fade: just send target color, hardware does the rest */
                target = black;
            } else if (use_fade && fade_progress < 1.0f) {
                /* Software fade: interpolate current color toward black */
                target = lerp_color(state->button_states[i].color, black, eased_progress);
            }
            
            state->button_states[i].color = target;
            
            /* Send to hardware */
            if (state->controllers && state->controller_count > 0) {
                ipac_set_led_all(state->controllers, state->controller_count,
                                (ButtonType)i, target);
            }
        }
    }
    
    /* Process buttons IN this frame */
    for (int b = 0; b < frame->button_count; b++) {
        ButtonColorPair *pair = &frame->buttons[b];
        int btn_idx = (int)pair->button;
        
        if (btn_idx >= 0 && btn_idx < state->total_buttons) {
            RGBColor target_color = pair->color;
            
            if (use_hw_fade) {
                /* Hardware fade: send final target color directly */
                /* The hardware will smoothly interpolate from current to target */
                target_color = pair->color;
            } else if (use_fade && fade_progress < 1.0f) {
                /* Software fade: interpolate from current color to target */
                RGBColor current = state->button_states[btn_idx].color;
                target_color = lerp_color(current, pair->color, eased_progress);
            }
            
            state->button_states[btn_idx].color = target_color;
            
            /* Send to specific controller or all */
            if (state->controllers && state->controller_count > 0) {
                if (pair->controller >= 0 && pair->controller < state->controller_count) {
                    ipac_set_led(&state->controllers[pair->controller], 
                                pair->button, target_color);
                } else {
                    ipac_set_led_all(state->controllers, state->controller_count,
                                    pair->button, target_color);
                }
            }
        }
    }
    
    /* Check if frame is complete */
    if (use_fade) {
        if (use_hw_fade) {
            /* With hardware fade, advance to next frame immediately
             * but wait for the fade duration before processing next frame */
            state->custom_frame_idx++;
            state->frame = 0;
        } else if (elapsed_ms >= fade_duration) {
            /* Software fade complete - advance to next frame */
            state->custom_frame_idx++;
            state->frame = 0;
        } else {
            state->frame++;
        }
    } else {
        /* Non-fading frame - advance immediately */
        state->custom_frame_idx++;
        state->frame = 0;
    }
}

/* Free animation configuration */
void free_animation_config(AnimationConfig *config) {
    if (config) {
        free(config->colors);
        free(config->buttons);
        free(config);
    }
}
