#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <time.h> // Required for struct timespec timeout
#include <sys/system_properties.h>
#include <android/log.h>

#define PROP_NAME "debug.tracing.screen_brightness"
#define STATE_PROP "debug.tracing.screen_state"
#define BACKLIGHT_PATH "/sys/class/leds/lcd-backlight/brightness"
#define LOG_TAG "YamadaOBright"

// Translation Logic based on Yamada Blueprint
int calculate_brightness(float prop_val) {
    // --- Settings Slider Fix (Float Mode) ---
    // If the framework writes a 0.0 to 1.0 float (from the Settings app),
    // scale it up to the framework integer range (222 to 8191) first.
    if (prop_val > 0.0f && prop_val <= 1.0f) {
        prop_val = 222.0f + (prop_val * (8191.0f - 222.0f));
    }
    // ----------------------------------------

    // Hardware constraints
    if (prop_val <= 222.0f) return 1;
    if (prop_val >= 8191.0f) return 4095;
    
    // 1. Normalize the framework value (0.0 to 1.0)
    float normalized = prop_val / 8191.0f;
    
    // 2. Reverse Android's gamma spline (approx ^3 curve) using a cube root
    // This extracts the actual linear slider percentage (e.g., 1008 -> ~0.5)
    float linear_percentage = cbrtf(normalized);
    
    // 3. Map the linear percentage to the 12-bit hardware scale
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
        // Log errors to logcat (linked via -llog)
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

// Helper to get the current screen state synchronously
int get_screen_state() {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(STATE_PROP, value) > 0) {
        return atoi(value);
    }
    return 2; // Default to ON (2) if the property doesn't exist yet
}

int main() {
    const prop_info *pi;
    uint32_t serial = 0;
    uint32_t new_serial = 0;
    float current_prop_val = 0.0f;
    
    // State tracking to prevent hardware spam
    int last_written_val = -1;
    int last_state = -1;

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Starting Yamada OPlus Display Adaptor with IPS Fix...");

    // 1. Initialization: Wait for the target property to exist
    while ((pi = __system_property_find(PROP_NAME)) == NULL) {
        usleep(500000); // 500ms
    }

    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Found target property. Entering hybrid listener mode.");

    // We use a 100ms timeout for __system_property_wait. 
    // This keeps CPU usage near 0% but ensures we catch screen_state changes 
    // even if screen_brightness doesn't trigger an event when turning off.
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100000000; // 100ms

    // 2. Hybrid Event/Polling Loop
    for (;;) {
        bool prop_changed = __system_property_wait(pi, serial, &new_serial, &timeout);
        
        if (prop_changed) {
            serial = new_serial;
            __system_property_read_callback(pi, read_prop_callback, &current_prop_val);
        }

        int current_state = get_screen_state();
        bool state_changed = (current_state != last_state);

        // Only process if something actually updated (brightness value or screen state)
        if (prop_changed || state_changed) {
            int new_brightness = last_written_val; // Default to keep the same

            if (current_state != 2) {
                // --- IPS FIX ---
                // Screen state is 0 (OFF), 1 (AOD), 3 (DOZE), or 4 (DOZE_SUSPEND)
                // Force the hardware backlight to 0 to prevent the "dim" IPS screen issue.
                new_brightness = 0;
                
                if (state_changed) {
                    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Screen state changed to %d. Forcing brightness to 0 (IPS Fix).", current_state);
                }
            } else {
                // --- SCREEN IS ON (2) ---
                if (current_prop_val == 0.0f) {
                    // OS16 Dim Record Patch
                    if (prop_changed) {
                        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "Brightness reported to be 0? ignoring and keeping previous value.");
                    }
                    
                    // Safety fallback: if we just turned ON from OFF, but property says 0.0, 
                    // write a safe minimum so the screen doesn't stay black.
                    if (state_changed && last_written_val <= 0) {
                        new_brightness = 1;
                    }
                } else {
                    // Normal translation
                    new_brightness = calculate_brightness(current_prop_val);
                }
            }

            // Prevent unnecessary writes to the hardware node
            if (new_brightness != last_written_val && new_brightness != -1) {
                write_backlight(new_brightness);
                last_written_val = new_brightness;
            }

            last_state = current_state;
        }
    }

    return 0;
}