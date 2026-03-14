/*
 * XTENSEI-OBright - OPlus Brightness Adaptor for Transsion Devices
 * 
 * A brightness fix for OPlus ports on Transsion devices (TECNO, Infinix, iTel)
 * Addresses screen dimming issues on IPS displays
 * 
 * Inspired by @ryanistr OPlus-Brightness-Adaptor-For-Transsion
 * 
 * Copyright (C) 2026 XTENSEI
 * Licensed under GPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <sys/system_properties.h>
#include <android/log.h>
#include <string.h>
#include <errno.h>

/* Property Configuration */
#define PROP_NAME           "debug.tracing.screen_brightness"
#define STATE_PROP          "debug.tracing.screen_state"
#define LOG_TAG             "XTENSEI-OBright"

/* Backlight Paths - Primary */
#define BACKLIGHT_PATH      "/sys/class/leds/lcd-backlight/brightness"
#define MAX_BRIGHT_PATH     "/sys/class/leds/lcd-backlight/max_hw_brightness"
#define MIN_BRIGHT_PATH     "/sys/class/leds/lcd-backlight/min_brightness"

/* Backlight Paths - Alternative (for devices with different paths) */
#define BACKLIGHT_PATH_ALT  "/sys/class/backlight/panel0-backlight/brightness"
#define MAX_BRIGHT_PATH_ALT "/sys/class/backlight/panel0-backlight/max_brightness"
#define MIN_BRIGHT_PATH_ALT "/sys/class/backlight/panel0-backlight/min_brightness"

/* Brightness Calculation Constants */
#define BRIGHTNESS_MIN_THRESHOLD  222.0f
#define BRIGHTNESS_MAX_VALUE      8191.0f
#define SCREEN_ON_STATE           2

/* Timing Constants */
#define HARDWARE_WAKE_DELAY_US    100000  /* 100ms delay for panel initialization */

/* ============================================================================
 * File I/O Helper Functions
 * ============================================================================ */

/**
 * Read integer value from a sysfs file
 * @param path Path to the file
 * @param default_val Default value if read fails
 * @return Integer value from file or default
 */
static int read_int_from_file(const char* path, int default_val) {
    if (!path) return default_val;
    
    FILE* f = fopen(path, "r");
    if (!f) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, 
                           "Failed to open %s: %s", path, strerror(errno));
        return default_val;
    }
    
    int val = default_val;
    if (fscanf(f, "%d", &val) != 1) {
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, 
                           "Failed to parse value from %s", path);
        val = default_val;
    }
    
    fclose(f);
    return val;
}

/**
 * Write brightness value to backlight control
 * @param brightness_val Brightness value to write
 * @return true on success, false on failure
 */
static bool write_backlight(int brightness_val) {
    FILE *f = fopen(BACKLIGHT_PATH, "w");
    if (!f) {
        /* Try alternative path */
        f = fopen(BACKLIGHT_PATH_ALT, "w");
        if (!f) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, 
                               "Failed to open backlight path: %s", strerror(errno));
            return false;
        }
    }
    
    if (fprintf(f, "%d\n", brightness_val) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, 
                           "Failed to write brightness value: %s", strerror(errno));
        fclose(f);
        return false;
    }
    
    fclose(f);
    return true;
}

/* ============================================================================
 * System Property Helper Functions
 * ============================================================================ */

/**
 * Read float system property
 * @param prop_name Property name
 * @param default_val Default value if property not found
 * @return Property value or default
 */
static float get_float_prop(const char* prop_name, float default_val) {
    if (!prop_name) return default_val;
    
    char value[PROP_VALUE_MAX];
    int len = __system_property_get(prop_name, value);
    
    if (len > 0) {
        char* endptr;
        float val = strtof(value, &endptr);
        if (*endptr == '\0' || *endptr == '\n') {
            return val;
        }
    }
    
    return default_val;
}

/**
 * Get current screen state
 * @return Screen state (0=OFF, 1=DOZE, 2=ON, 3=AOD)
 */
static int get_screen_state(void) {
    char value[PROP_VALUE_MAX];
    int len = __system_property_get(STATE_PROP, value);
    
    if (len > 0) {
        char* endptr;
        long state = strtol(value, &endptr, 10);
        if (*endptr == '\0' || *endptr == '\n') {
            return (int)state;
        }
    }
    
    return SCREEN_ON_STATE; /* Default to ON */
}

/* ============================================================================
 * Brightness Calculation
 * ============================================================================ */

/**
 * Calculate brightness value based on property and hardware constraints
 * 
 * Implements cube root mapping for better perceptual brightness control:
 * - Settings slider values (0.0-1.0) are mapped to hardware range
 * - Uses cube root curve for more natural brightness perception
 * - Respects hardware min/max constraints
 * 
 * @param prop_val Property value from settings
 * @param hw_min Hardware minimum brightness
 * @param hw_max Hardware maximum brightness
 * @return Calculated brightness value
 */
static int calculate_brightness(float prop_val, int hw_min, int hw_max) {
    /* Validate hardware constraints */
    if (hw_min >= hw_max) {
        hw_min = 1;
        hw_max = 4095;
    }
    
    /* Settings Slider Fix: Map 0.0-1.0 range to hardware range */
    if (prop_val > 0.0f && prop_val <= 1.0f) {
        prop_val = BRIGHTNESS_MIN_THRESHOLD + 
                   (prop_val * (BRIGHTNESS_MAX_VALUE - BRIGHTNESS_MIN_THRESHOLD));
    }

    /* Handle edge cases */
    if (prop_val <= BRIGHTNESS_MIN_THRESHOLD) {
        return hw_min;
    }
    if (prop_val >= BRIGHTNESS_MAX_VALUE) {
        return hw_max;
    }

    /* Cube root mapping for perceptual linearity */
    float normalized = prop_val / BRIGHTNESS_MAX_VALUE;
    float linear_percentage = cbrtf(normalized);
    float mapped = linear_percentage * (float)hw_max;

    int result = (int)roundf(mapped);
    
    /* Clamp to hardware constraints */
    if (result < hw_min) return hw_min;
    if (result > hw_max) return hw_max;
    
    return result;
}

/* ============================================================================
 * Main Service Entry Point
 * ============================================================================ */

int main(void) {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                       "XTENSEI OPlus Display Adaptor Starting (Event Trigger Mode)...");
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                       "Copyright (C) 2026 XTENSEI - Licensed under GPL-3.0");

    /* Step 1: Read Hardware Min/Max Constraints */
    int hw_max = read_int_from_file(MAX_BRIGHT_PATH, 4095);
    int hw_min = read_int_from_file(MIN_BRIGHT_PATH, 1);
    
    /* Try alternative paths if primary failed */
    if (hw_max == 4095) {
        int alt_max = read_int_from_file(MAX_BRIGHT_PATH_ALT, 0);
        if (alt_max > 0) {
            hw_max = alt_max;
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                               "Using alternative backlight path");
        }
    }
    
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                       "Hardware constraints: Min=%d, Max=%d", hw_min, hw_max);

    /* Step 2: Setup Event Trigger (Global Serial Tracker) */
    uint32_t global_serial = 0;
    struct timespec no_wait = {0, 0};

    /* Get initial global serial state before entering loop */
    __system_property_wait(NULL, 0, &global_serial, &no_wait);

    /* Step 3: Initialize State */
    float current_prop_val = get_float_prop(PROP_NAME, 0.0f);
    int prev_state = get_screen_state();

    int raw_initial = (current_prop_val == 0.0f) ? -1 : 
                      calculate_brightness(current_prop_val, hw_min, hw_max);
    int prev_bright = (raw_initial == -1) ? hw_min : raw_initial;
    int last_written_val = -1;

    /* Apply initial state */
    if (prev_state != SCREEN_ON_STATE) {
        last_written_val = 0;
        write_backlight(0);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                           "Initial state: Screen OFF, brightness=0");
    } else {
        last_written_val = prev_bright;
        write_backlight(prev_bright);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                           "Initial state: Screen ON, brightness=%d", prev_bright);
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, 
                       "Entering Event-Driven Loop (0%% CPU while idle)");

    /* Step 4: Pure Event-Driven Loop */
    for (;;) {
        /* Block until ANY system property changes - 0% CPU usage while waiting */
        __system_property_wait(NULL, global_serial, &global_serial, NULL);

        float new_prop_val = get_float_prop(PROP_NAME, 0.0f);
        int cur_state = get_screen_state();

        /* Cache Mechanism: Use last known good brightness for 0.0f values */
        int raw_bright = (new_prop_val == 0.0f) ? -1 : 
                         calculate_brightness(new_prop_val, hw_min, hw_max);
        int cur_bright = (raw_bright == -1) ? prev_bright : raw_bright;
        if (cur_bright == -1) cur_bright = hw_min;

        /* Check if state or brightness changed */
        if (cur_bright != prev_bright || cur_state != prev_state) {
            int val_to_write = last_written_val;

            if (cur_state != SCREEN_ON_STATE) {
                /* Screen OFF/DOZE/AOD: Force 0 for IPS panel fix */
                val_to_write = 0;
                __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, 
                                   "Screen state change: OFF -> brightness=0");
            } else {
                /* Screen ON */
                if (prev_state != SCREEN_ON_STATE) {
                    /* Hardware wake delay for panel initialization */
                    usleep(HARDWARE_WAKE_DELAY_US);
                    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, 
                                       "Screen state change: ON -> brightness=%d", 
                                       cur_bright);
                }
                val_to_write = cur_bright;
            }

            /* Write only if value changed */
            if (val_to_write != last_written_val) {
                if (write_backlight(val_to_write)) {
                    last_written_val = val_to_write;
                }
            }
        }

        /* Update state tracking */
        prev_bright = cur_bright;
        prev_state = cur_state;
        current_prop_val = new_prop_val;
    }

    return 0; /* Never reached */
}
