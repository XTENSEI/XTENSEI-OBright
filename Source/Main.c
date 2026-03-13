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
#define LOG_TAG "YamadaOBright"

// Translation Logic
int calculate_brightness(float prop_val) {
    // --- Settings Slider Fix (Float Mode) ---
    if (prop_val > 0.0f && prop_val <= 1.0f) {
        prop_val = 222.0f + (prop_val * (8191.0f - 222.0f));
    }

    // Hardware constraints
    if (prop_val <= 222.0f) return 1;
    if (prop_val >= 8191.0f) return 4095;
    
    float normalized = prop_val / 8191.0f;
    float linear_percentage = cbrtf(normalized);
    float mapped = linear_percentage * 4095.0f;
    
    return (int)roundf(mapped);
}

// Hardware Write Function
void write_backlight(int brightness_val) {
    FILE *f = fopen(BACKLIGHT_PATH, "w");
    if (f != NULL) {
        fprintf(f, "%d\n", brightness_val);
        fclose(f);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to write to %s", BACKLIGHT_PATH);
    }
}

// Callback to extract the float value from the property
void read_prop_callback(void* cookie, const char* name, const char* value, uint32_t serial) {
    float* out_val = (float*)cookie;
    if (value != NULL) {
        *out_val = atof(value);
    }
}

// Helper to get the current screen state
int get_screen_state() {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(STATE_PROP, value) > 0) {
        return atoi(value);
    }
    return 2; // Default to ON (2) if the property doesn't exist yet
}

int main() {
    const prop_info *pi;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Starting Yamada OPlus Display Adaptor (Polling & IPS Fix)...");

    // 1. Initialization: Wait for the target property to exist
    while ((pi = __system_property_find(PROP_NAME)) == NULL) {
        usleep(500000); // 500ms
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Found target property. Entering 100ms polling loop.");

    // State tracking to prevent hardware spam and handle wake drops
    int last_written_val = -1;
    int prev_state = get_screen_state();
    int prev_bright = -1;

    // 2. Pure Polling Loop (Bypasses the Kernel Futex Suspend Bug)
    for (;;) {
        float current_prop_val = 0.0f;
        __system_property_read_callback(pi, read_prop_callback, &current_prop_val);

        // Convert to hardware scale, flag 0.0f as -1 so we can ignore it
        int raw_bright = (current_prop_val == 0.0f) ? -1 : calculate_brightness(current_prop_val);
        int cur_state = get_screen_state();
        
        // Cache Mechanism: Ignore 0.0f and use our last known good brightness.
        int cur_bright = (raw_bright == -1) ? prev_bright : raw_bright;
        
        // Fallback for the absolute first loop if device boots with screen off
        if (cur_bright == -1) cur_bright = 1;

        // Only process if brightness or state changed
        if (cur_bright != prev_bright || cur_state != prev_state) {
            int val_to_write = last_written_val;

            if (cur_state != 2) {
                // OFF, DOZE, AOD -> Force 0 for IPS Fix
                val_to_write = 0;
            } else {
                // SCREEN IS ON (2)
                if (prev_state != 2) {
                    // HARDWARE WAKE DELAY
                    // The display panel needs a fraction of a second to initialize 
                    // before it can accept backlight commands.
                    usleep(100000); // Wait 100ms
                }
                val_to_write = cur_bright;
            }

            // Prevent unnecessary writes to the hardware node
            if (val_to_write != last_written_val) {
                write_backlight(val_to_write);
                last_written_val = val_to_write;
            }
        }

        // Update caches for the next loop
        prev_bright = cur_bright;
        prev_state = cur_state;

        // 100ms polling interval. Uses virtually 0% CPU but survives deep sleep flawlessly.
        usleep(100000); 
    }

    return 0;
}