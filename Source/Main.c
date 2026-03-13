#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <sys/system_properties.h>
#include <android/log.h>

#define PROP_NAME "debug.tracing.screen_brightness"
#define STATE_PROP "debug.tracing.screen_state"
#define BACKLIGHT_PATH "/sys/class/leds/lcd-backlight/brightness"
#define MAX_BRIGHT_PATH "/sys/class/leds/lcd-backlight/max_hw_brightness"
#define MIN_BRIGHT_PATH "/sys/class/leds/lcd-backlight/min_brightness"
#define LOG_TAG "YamadaOBright"

// --- File Reading Helpers ---
int read_int_from_file(const char* path, int default_val) {
    FILE* f = fopen(path, "r");
    if (!f) return default_val;
    int val = default_val;
    if (fscanf(f, "%d", &val) != 1) {
        val = default_val;
    }
    fclose(f);
    return val;
}

void write_backlight(int brightness_val) {
    FILE *f = fopen(BACKLIGHT_PATH, "w");
    if (f != NULL) {
        fprintf(f, "%d\n", brightness_val);
        fclose(f);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to write to %s", BACKLIGHT_PATH);
    }
}

// --- Property Reading Helpers ---
float get_float_prop(const char* prop_name, float default_val) {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(prop_name, value) > 0) {
        return atof(value);
    }
    return default_val;
}

int get_screen_state() {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(STATE_PROP, value) > 0) {
        return atoi(value);
    }
    return 2; // Default to ON (2)
}

// --- Math Translation ---
int calculate_brightness(float prop_val, int hw_min, int hw_max) {
    // Settings Slider Fix
    if (prop_val > 0.0f && prop_val <= 1.0f) {
        prop_val = 222.0f + (prop_val * (8191.0f - 222.0f));
    }

    // Hardware constraints using dynamic paths
    if (prop_val <= 222.0f) return hw_min;
    if (prop_val >= 8191.0f) return hw_max;
    
    float normalized = prop_val / 8191.0f;
    float linear_percentage = cbrtf(normalized);
    float mapped = linear_percentage * (float)hw_max;
    
    int result = (int)roundf(mapped);
    if (result < hw_min) return hw_min;
    if (result > hw_max) return hw_max;
    return result;
}

// --- Main Daemon ---
int main() {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Starting Yamada OPlus Display Adaptor (Event Trigger Mode)...");

    // 1. Read Hardware Min/Max
    int hw_max = read_int_from_file(MAX_BRIGHT_PATH, 4095); // Fallback to 4095 if missing
    int hw_min = read_int_from_file(MIN_BRIGHT_PATH, 1);    // Fallback to 1 if missing
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hardware constraints loaded: Min=%d, Max=%d", hw_min, hw_max);

    // 2. Setup Event Trigger (Global Serial Tracker)
    uint32_t global_serial = 0;
    struct timespec no_wait = {0, 0};
    
    // Non-blocking call to get the current global serial state before entering loop
    __system_property_wait(NULL, 0, &global_serial, &no_wait); 

    // 3. Initialize Starting State
    float current_prop_val = get_float_prop(PROP_NAME, 0.0f);
    int prev_state = get_screen_state();
    
    int raw_initial = (current_prop_val == 0.0f) ? -1 : calculate_brightness(current_prop_val, hw_min, hw_max);
    int prev_bright = (raw_initial == -1) ? hw_min : raw_initial;
    int last_written_val = -1;

    // Apply immediate start state
    if (prev_state != 2) {
        last_written_val = 0;
        write_backlight(0);
    } else {
        last_written_val = prev_bright;
        write_backlight(prev_bright);
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Entering Blocking Event Loop.");

    // 4. Pure Event-Driven Loop (No Polling / No sleep timers)
    for (;;) {
        // This blocks the thread completely until ANY system property changes. 
        // 0% CPU usage while waiting.
        __system_property_wait(NULL, global_serial, &global_serial, NULL);

        float new_prop_val = get_float_prop(PROP_NAME, 0.0f);
        int cur_state = get_screen_state();

        // Cache Mechanism: Ignore 0.0f and use our last known good brightness.
        int raw_bright = (new_prop_val == 0.0f) ? -1 : calculate_brightness(new_prop_val, hw_min, hw_max);
        int cur_bright = (raw_bright == -1) ? prev_bright : raw_bright;
        if (cur_bright == -1) cur_bright = hw_min;

        // If either the screen state changed or the brightness slider moved
        if (cur_bright != prev_bright || cur_state != prev_state) {
            int val_to_write = last_written_val;

            if (cur_state != 2) {
                // OFF, DOZE, AOD -> Force 0 for IPS Fix
                val_to_write = 0;
            } else {
                // SCREEN IS ON (2)
                if (prev_state != 2) {
                    // HARDWARE WAKE DELAY
                    // Sequential delay required for panel initialization.
                    usleep(100000); 
                }
                val_to_write = cur_bright;
            }

            // Write and update cache
            if (val_to_write != last_written_val) {
                write_backlight(val_to_write);
                last_written_val = val_to_write;
            }
        }

        // Update state tracking
        prev_bright = cur_bright;
        prev_state = cur_state;
        current_prop_val = new_prop_val;
    }

    return 0;
}