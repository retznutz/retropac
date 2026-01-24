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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
AnimationState *animation_create(AnimationConfig *config, int ipac_handle, 
                                  PinMapping *pin_mappings,
                                  ButtonConfig *initial_buttons, int button_count) {
    AnimationState *state = calloc(1, sizeof(AnimationState));
    if (!state) return NULL;
    
    state->config = config;
    state->ipac_handle = ipac_handle;
    state->pin_mappings = pin_mappings;
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

/* Breathing/pulse animation frame */
static void animate_breathing(AnimationState *state) {
    /* Sine wave for smooth breathing: period = ~120 frames */
    float phase = (state->frame % 120) / 120.0f * 2.0f * M_PI;
    float brightness = (sinf(phase) + 1.0f) / 2.0f;  /* 0.0 to 1.0 */
    
    /* Ensure minimum brightness so LEDs don't go completely off */
    brightness = 0.1f + (brightness * 0.9f);
    
    RGBColor base = state->config->base_color;
    RGBColor scaled = scale_color(base, brightness);
    
    for (int i = 0; i < state->total_buttons; i++) {
        state->button_states[i].color = scaled;
    }
}

/* Chase/running light animation frame */
static void animate_chase(AnimationState *state) {
    int active_pos = state->frame % state->total_buttons;
    int tail_length = 3;
    
    RGBColor off = {0, 0, 0};
    RGBColor base = state->config->base_color;
    
    for (int i = 0; i < state->total_buttons; i++) {
        int distance = (active_pos - i + state->total_buttons) % state->total_buttons;
        
        if (distance == 0) {
            state->button_states[i].color = base;
        } else if (distance <= tail_length) {
            float fade = 1.0f - ((float)distance / (tail_length + 1));
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
    
    /* Send updated colors to hardware */
    if (state->ipac_handle >= 0) {
        for (int i = 0; i < state->total_buttons; i++) {
            ipac_set_led(state->ipac_handle, 
                        state->button_states[i].button,
                        state->button_states[i].color,
                        state->pin_mappings);
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
    
    state->running = false;
    printf("Animation stopped\n");
}

/* Create animation state for custom animation */
AnimationState *animation_create_custom(CustomAnimation *custom_anim, int ipac_handle,
                                         PinMapping *pin_mappings,
                                         ButtonConfig *initial_buttons, int button_count) {
    (void)initial_buttons;  /* Not used - we create all buttons */
    (void)button_count;     /* Not used - we use BUTTON_MAX */
    
    if (!custom_anim) return NULL;
    
    AnimationState *state = calloc(1, sizeof(AnimationState));
    if (!state) return NULL;
    
    state->config = NULL;  /* No built-in config */
    state->custom_anim = custom_anim;
    state->ipac_handle = ipac_handle;
    state->pin_mappings = pin_mappings;
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
        
        /* Only send to hardware if button has valid pin mappings */
        if (ipac_handle >= 0 && pin_mappings) {
            PinMapping *pins = &pin_mappings[i];
            if (pins->r_pin >= 0 && pins->g_pin >= 0 && pins->b_pin >= 0) {
                ipac_set_led(ipac_handle, (ButtonType)i, off, pin_mappings);
            }
        }
    }
    
    return state;
}

/* Linear interpolation between colors */
static RGBColor lerp_color(RGBColor from, RGBColor to, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
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
    
    /* Calculate time spent on current frame */
    int frame_time = state->frame * anim->speed_ms;
    
    /* At the start of a new frame (frame_time == 0), turn off all buttons not in this frame */
    if (frame_time == 0) {
        for (int i = 0; i < state->total_buttons; i++) {
            /* Check if this button is in the current frame */
            bool in_frame = false;
            for (int b = 0; b < frame->button_count; b++) {
                if (frame->buttons[b].button == (ButtonType)i) {
                    in_frame = true;
                    break;
                }
            }
            /* If not in frame, set target to black */
            if (!in_frame) {
                RGBColor black = {0, 0, 0};
                state->button_states[i].color = black;
            }
        }
    }
    
    /* Process all buttons in this frame */
    for (int b = 0; b < frame->button_count; b++) {
        ButtonColorPair *pair = &frame->buttons[b];
        
        /* Button index is same as button enum value */
        int btn_idx = (int)pair->button;
        if (btn_idx >= 0 && btn_idx < state->total_buttons) {
            RGBColor target_color = pair->color;
            
            if (frame->fade && frame->fade_speed_ms > 0) {
                /* Calculate fade progress */
                float fade_progress = (float)frame_time / frame->fade_speed_ms;
                
                if (fade_progress < 1.0f) {
                    /* Still fading - interpolate color */
                    RGBColor current = state->button_states[btn_idx].color;
                    target_color = lerp_color(current, pair->color, fade_progress);
                }
            }
            
            state->button_states[btn_idx].color = target_color;
        }
    }
    
    /* Send updated colors to hardware - only for buttons that have pin mappings */
    if (state->ipac_handle >= 0 && state->pin_mappings) {
        for (int i = 0; i < state->total_buttons; i++) {
            /* Skip buttons without valid pin mappings */
            PinMapping *pins = &state->pin_mappings[i];
            if (pins->r_pin < 0 || pins->g_pin < 0 || pins->b_pin < 0) {
                continue;
            }
            
            ipac_set_led(state->ipac_handle,
                        state->button_states[i].button,
                        state->button_states[i].color,
                        state->pin_mappings);
        }
    }
    
    /* Determine frame duration and check if complete */
    if (frame->fade && frame->fade_speed_ms > 0) {
        /* Fading frame - check if fade is complete */
        int frame_duration = frame->fade_speed_ms;
        if (frame_time >= frame_duration) {
            state->custom_frame_idx++;
            state->frame = 0; /* Reset frame counter for next animation frame */
        } else {
            state->frame++;
        }
    } else {
        /* Non-fading frame - advance immediately after setting colors */
        /* The main loop's sleep handles the timing */
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
