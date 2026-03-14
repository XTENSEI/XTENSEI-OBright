/*
 * XTENSEI-OBright - Universal OPlus Brightness Adaptor for Transsion Devices
 * 
 * The ultimate brightness control solution for Transsion devices (TECNO, Infinix, iTel)
 * with OPlus-based custom ROMs. Features universal backlight path support, advanced
 * brightness algorithms, and device-specific profiles.
 * 
 * Inspired by @ryanistr OPlus-Brightness-Adaptor-For-Transsion
 * 
 * Copyright (C) 2026 XTENSEI <andriegalutera@gmail.com>
 * Licensed under GPL-3.0
 * 
 * Features:
 * - Universal backlight path detection (20+ paths)
 * - Device-specific brightness profiles
 * - Multiple brightness curve algorithms
 * - Real-time configuration reloading
 * - Comprehensive logging and debugging
 * - Proper daemon lifecycle management
 * - Thermal throttling awareness
 * - Battery saver integration
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
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

/* ============================================================================
 * Version Information
 * ============================================================================ */
#define XTENSEI_VERSION_MAJOR     2
#define XTENSEI_VERSION_MINOR     0
#define XTENSEI_VERSION_PATCH     0
#define XTENSEI_VERSION_STRING    "2.0.0-universal"
#define XTENSEI_CODENAME          "Phoenix"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define LOG_TAG                     "XTENSEI-OBright"
#define PROP_NAME                   "debug.tracing.screen_brightness"
#define STATE_PROP                  "debug.tracing.screen_state"
#define BATTERY_SAVER_PROP          "sys.power.saving"
#define THERMAL_THROTTLE_PROP       "sys.thermal.throttle"

/* Configuration file paths */
#define CONFIG_PATH_PRIMARY         "/vendor/etc/xtensei-obright.conf"
#define CONFIG_PATH_SECONDARY       "/system/etc/xtensei-obright.conf"
#define CONFIG_PATH_USER            "/data/local/tmp/xtensei-obright.conf"

/* Log file path */
#define LOG_FILE_PATH               "/data/local/tmp/xtensei-obright.log"

/* ============================================================================
 * Backlight Paths - Universal Support (20+ paths for maximum compatibility)
 * ============================================================================ */
static const char* BACKLIGHT_PATHS[] = {
    /* Primary LCD paths */
    "/sys/class/leds/lcd-backlight/brightness",
    "/sys/class/backlight/panel0-backlight/brightness",
    "/sys/class/backlight/lcd-backlight/brightness",
    
    /* MediaTek paths */
    "/sys/class/leds/lcd-backlight0/brightness",
    "/sys/class/backlight/mtk-backlight/brightness",
    "/sys/class/backlight/mtk_leds/brightness",
    
    /* Qualcomm paths */
    "/sys/class/backlight/qcom-backlight/brightness",
    "/sys/class/leds/lcd-backlight-qcom/brightness",
    
    /* Transsion-specific paths (TECNO, Infinix, iTel) */
    "/sys/class/leds/transsion-backlight/brightness",
    "/sys/class/backlight/transsion-lcd/brightness",
    "/sys/class/leds/tecno-backlight/brightness",
    "/sys/class/backlight/infinix-lcd/brightness",
    "/sys/class/leds/itel-backlight/brightness",
    
    /* OPlus/Realme paths */
    "/sys/class/backlight/oplus-backlight/brightness",
    "/sys/class/leds/oplus-lcd/brightness",
    
    /* Alternative common paths */
    "/sys/class/backlight/backlight/brightness",
    "/sys/class/leds/backlight/brightness",
    "/sys/class/backlight/platform-backlight/brightness",
    "/sys/class/leds/platform-backlight/brightness",
    
    /* Deep alternative paths */
    "/sys/devices/platform/leds/leds/lcd-backlight/brightness",
    "/sys/devices/virtual/backlight/backlight/brightness",
    "/sys/class/backlight/pwm-backlight/brightness",
    
    NULL  /* Sentinel */
};

static const char* MAX_BRIGHT_PATHS[] = {
    "/sys/class/leds/lcd-backlight/max_hw_brightness",
    "/sys/class/backlight/panel0-backlight/max_brightness",
    "/sys/class/backlight/lcd-backlight/max_brightness",
    "/sys/class/leds/lcd-backlight0/max_brightness",
    "/sys/class/backlight/mtk-backlight/max_brightness",
    "/sys/class/backlight/qcom-backlight/max_brightness",
    "/sys/class/leds/transsion-backlight/max_brightness",
    "/sys/class/backlight/oplus-backlight/max_brightness",
    "/sys/class/backlight/backlight/max_brightness",
    "/sys/class/leds/backlight/max_brightness",
    NULL  /* Sentinel */
};

static const char* MIN_BRIGHT_PATHS[] = {
    "/sys/class/leds/lcd-backlight/min_brightness",
    "/sys/class/backlight/panel0-backlight/min_brightness",
    "/sys/class/backlight/lcd-backlight/min_brightness",
    "/sys/class/leds/lcd-backlight0/min_brightness",
    NULL  /* Sentinel */
};

/* ============================================================================
 * Brightness Constants
 * ============================================================================ */
#define BRIGHTNESS_MIN_THRESHOLD      222.0f
#define BRIGHTNESS_MAX_VALUE          8191.0f
#define BRIGHTNESS_SCREEN_ON_STATE    2
#define BRIGHTNESS_SCREEN_OFF_STATE   0
#define BRIGHTNESS_SCREEN_DOZE_STATE  1
#define BRIGHTNESS_SCREEN_AOD_STATE   3

#define HARDWARE_WAKE_DELAY_US        100000
#define CONFIG_RELOAD_INTERVAL_SEC    30
#define LOG_BUFFER_SIZE               4096

/* ============================================================================
 * Brightness Curve Types
 * ============================================================================ */
typedef enum {
    CURVE_CUBE_ROOT,        /* Default - perceptual linearity */
    CURVE_LINEAR,           /* Direct mapping */
    CURVE_GAMMA_22,         /* Gamma 2.2 correction */
    CURVE_GAMMA_24,         /* Gamma 2.4 correction */
    CURVE_LOGARITHMIC,      /* Log curve for dark rooms */
    CURVE_EXPONENTIAL,      /* Exponential for bright environments */
    CURVE_S_CURVE,          /* S-curve for balanced response */
    CURVE_CUSTOM            /* User-defined curve */
} BrightnessCurve;

/* ============================================================================
 * Device Profiles
 * ============================================================================ */
typedef struct {
    const char* device_codename;
    const char* device_name;
    const char* manufacturer;
    int max_brightness;
    int min_brightness;
    int default_curve;
    int wake_delay_ms;
    bool has_thermal_throttle;
    bool has_battery_saver_integration;
} DeviceProfile;

/* Device profile database for Transsion devices */
static const DeviceProfile DEVICE_PROFILES[] = {
    /* TECNO Devices */
    {"TECNO-LH8n", "TECNO Spark 10 Pro", "TECNO", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"TECNO-LH9", "TECNO Spark 10C", "TECNO", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"TECNO-LJ8", "TECNO Camon 20", "TECNO", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"TECNO-LJ9", "TECNO Camon 20 Pro", "TECNO", 4095, 1, CURVE_GAMMA_22, 100, true, true},
    {"TECNO-LK1", "TECNO Phantom X2", "TECNO", 4095, 2, CURVE_GAMMA_22, 100, true, true},
    {"TECNO-LK2", "TECNO Phantom X2 Pro", "TECNO", 4095, 2, CURVE_GAMMA_22, 100, true, true},
    {"TECNO-LM1", "TECNO Pova 5", "TECNO", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"TECNO-LM2", "TECNO Pova 5 Pro", "TECNO", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    
    /* Infinix Devices */
    {"Infinix-X6833", "Infinix Note 30", "Infinix", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"Infinix-X6834", "Infinix Note 30 Pro", "Infinix", 4095, 1, CURVE_GAMMA_22, 100, true, true},
    {"Infinix-X6831", "Infinix Hot 30", "Infinix", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"Infinix-X6832", "Infinix Hot 30i", "Infinix", 4095, 1, CURVE_CUBE_ROOT, 100, true, true},
    {"Infinix-X6816", "Infinix Zero Ultra", "Infinix", 4095, 2, CURVE_GAMMA_24, 100, true, true},
    {"Infinix-X6820", "Infinix GT 10 Pro", "Infinix", 4095, 1, CURVE_S_CURVE, 100, true, true},
    
    /* iTel Devices */
    {"itel-A70", "itel A70", "itel", 2047, 1, CURVE_LINEAR, 100, false, true},
    {"itel-S23", "itel S23", "itel", 2047, 1, CURVE_LINEAR, 100, false, true},
    {"itel-P40", "itel P40", "itel", 2047, 1, CURVE_LINEAR, 100, false, true},
    
    /* Generic fallback */
    {NULL, "Generic Transsion Device", "Generic", 4095, 1, CURVE_CUBE_ROOT, 100, false, true}
};

/* ============================================================================
 * Global State
 * ============================================================================ */
typedef struct {
    /* Backlight paths */
    const char* brightness_path;
    const char* max_path;
    const char* min_path;
    
    /* Hardware constraints */
    int hw_max;
    int hw_min;
    
    /* Current state */
    float current_brightness;
    int screen_state;
    int last_written_value;
    
    /* Configuration */
    BrightnessCurve curve;
    bool battery_saver_active;
    bool thermal_throttle_active;
    float battery_saver_scale;
    float thermal_scale;
    
    /* Device profile */
    const DeviceProfile* profile;
    
    /* Runtime */
    bool running;
    time_t last_config_reload;
    int log_level;
} BrightnessState;

static BrightnessState g_state = {0};
static volatile sig_atomic_t g_signal_received = 0;

/* ============================================================================
 * Signal Handler
 * ============================================================================ */
static void signal_handler(int sig) {
    g_signal_received = sig;
    g_state.running = false;
}

/* ============================================================================
 * Logging Functions
 * ============================================================================ */
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_VERBOSE = 4
} LogLevel;

static void log_message(LogLevel level, const char* format, ...) {
    if (level > g_state.log_level) return;
    
    int android_level;
    switch (level) {
        case LOG_LEVEL_ERROR:   android_level = ANDROID_LOG_ERROR; break;
        case LOG_LEVEL_WARN:    android_level = ANDROID_LOG_WARN; break;
        case LOG_LEVEL_INFO:    android_level = ANDROID_LOG_INFO; break;
        case LOG_LEVEL_DEBUG:   android_level = ANDROID_LOG_DEBUG; break;
        case LOG_LEVEL_VERBOSE: android_level = ANDROID_LOG_VERBOSE; break;
        default:                android_level = ANDROID_LOG_INFO;
    }
    
    va_list args;
    va_start(args, format);
    __android_log_vprint(android_level, LOG_TAG, format, args);
    va_end(args);
}

#define LOG_E(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_W(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_I(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_D(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_V(...) log_message(LOG_LEVEL_VERBOSE, __VA_ARGS__)

/* ============================================================================
 * File I/O Helper Functions
 * ============================================================================ */

/**
 * Check if a file exists and is readable
 */
static bool file_exists(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

/**
 * Check if a sysfs path exists
 */
static bool sysfs_path_exists(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0);
}

/**
 * Read integer value from a sysfs file
 */
static int read_int_from_file(const char* path, int default_val) {
    if (!path) return default_val;
    
    FILE* f = fopen(path, "r");
    if (!f) {
        LOG_D("Failed to open %s: %s", path, strerror(errno));
        return default_val;
    }
    
    int val = default_val;
    if (fscanf(f, "%d", &val) != 1) {
        LOG_W("Failed to parse value from %s", path);
        val = default_val;
    }
    
    fclose(f);
    return val;
}

/**
 * Write integer value to a sysfs file
 */
static bool write_int_to_file(const char* path, int value) {
    if (!path) return false;
    
    FILE* f = fopen(path, "w");
    if (!f) {
        LOG_E("Failed to open %s for writing: %s", path, strerror(errno));
        return false;
    }
    
    if (fprintf(f, "%d\n", value) < 0) {
        LOG_E("Failed to write to %s: %s", path, strerror(errno));
        fclose(f);
        return false;
    }
    
    fclose(f);
    return true;
}

/**
 * Write brightness value to backlight control with path fallback
 */
static bool write_backlight(int brightness_val) {
    if (!g_state.brightness_path) {
        LOG_E("No brightness path configured");
        return false;
    }
    
    /* Try primary path */
    if (write_int_to_file(g_state.brightness_path, brightness_val)) {
        return true;
    }
    
    /* Try all fallback paths */
    for (int i = 0; BACKLIGHT_PATHS[i] != NULL; i++) {
        if (BACKLIGHT_PATHS[i] == g_state.brightness_path) continue;
        if (write_int_to_file(BACKLIGHT_PATHS[i], brightness_val)) {
            LOG_I("Successfully wrote to fallback path: %s", BACKLIGHT_PATHS[i]);
            g_state.brightness_path = BACKLIGHT_PATHS[i];
            return true;
        }
    }
    
    LOG_E("All backlight paths failed");
    return false;
}

/* ============================================================================
 * Path Detection
 * ============================================================================ */

/**
 * Auto-detect working backlight paths
 */
static bool detect_backlight_paths(void) {
    LOG_I("Detecting backlight paths...");
    
    /* Detect brightness path */
    for (int i = 0; BACKLIGHT_PATHS[i] != NULL; i++) {
        if (sysfs_path_exists(BACKLIGHT_PATHS[i])) {
            /* Test if writable */
            FILE* f = fopen(BACKLIGHT_PATHS[i], "w");
            if (f) {
                fclose(f);
                g_state.brightness_path = BACKLIGHT_PATHS[i];
                LOG_I("Found brightness path: %s", BACKLIGHT_PATHS[i]);
                break;
            }
        }
    }
    
    if (!g_state.brightness_path) {
        LOG_E("No writable brightness path found!");
        return false;
    }
    
    /* Detect max brightness path */
    for (int i = 0; MAX_BRIGHT_PATHS[i] != NULL; i++) {
        if (sysfs_path_exists(MAX_BRIGHT_PATHS[i])) {
            g_state.max_path = MAX_BRIGHT_PATHS[i];
            LOG_I("Found max brightness path: %s", MAX_BRIGHT_PATHS[i]);
            break;
        }
    }
    
    /* Detect min brightness path */
    for (int i = 0; MIN_BRIGHT_PATHS[i] != NULL; i++) {
        if (sysfs_path_exists(MIN_BRIGHT_PATHS[i])) {
            g_state.min_path = MIN_BRIGHT_PATHS[i];
            LOG_I("Found min brightness path: %s", MIN_BRIGHT_PATHS[i]);
            break;
        }
    }
    
    return true;
}

/* ============================================================================
 * System Property Helper Functions
 * ============================================================================ */

static float get_float_prop(const char* prop_name, float default_val) {
    if (!prop_name) return default_val;
    
    char value[PROP_VALUE_MAX];
    int len = __system_property_get(prop_name, value);
    
    if (len > 0) {
        char* endptr;
        float val = strtof(value, &endptr);
        if (*endptr == '\0' || *endptr == '\n' || *endptr == '.') {
            return val;
        }
    }
    
    return default_val;
}

static int get_int_prop(const char* prop_name, int default_val) {
    if (!prop_name) return default_val;
    
    char value[PROP_VALUE_MAX];
    int len = __system_property_get(prop_name, value);
    
    if (len > 0) {
        char* endptr;
        long val = strtol(value, &endptr, 10);
        if (*endptr == '\0' || *endptr == '\n') {
            return (int)val;
        }
    }
    
    return default_val;
}

static bool get_bool_prop(const char* prop_name, bool default_val) {
    if (!prop_name) return default_val;
    
    char value[PROP_VALUE_MAX];
    int len = __system_property_get(prop_name, value);
    
    if (len > 0) {
        if (strcmp(value, "1") == 0 || strcmp(value, "true") == 0 ||
            strcmp(value, "yes") == 0 || strcmp(value, "enabled") == 0) {
            return true;
        }
        if (strcmp(value, "0") == 0 || strcmp(value, "false") == 0 ||
            strcmp(value, "no") == 0 || strcmp(value, "disabled") == 0) {
            return false;
        }
    }
    
    return default_val;
}

static int get_screen_state(void) {
    int state = get_int_prop(STATE_PROP, BRIGHTNESS_SCREEN_ON_STATE);
    return state;
}

/* ============================================================================
 * Device Detection
 * ============================================================================ */

static const DeviceProfile* detect_device_profile(void) {
    char prop_value[PROP_VALUE_MAX];
    
    /* Try ro.product.device */
    if (__system_property_get("ro.product.device", prop_value) > 0) {
        for (size_t i = 0; DEVICE_PROFILES[i].device_codename != NULL; i++) {
            if (strcmp(prop_value, DEVICE_PROFILES[i].device_codename) == 0) {
                LOG_I("Device matched: %s (%s)", 
                      DEVICE_PROFILES[i].device_name,
                      DEVICE_PROFILES[i].device_codename);
                return &DEVICE_PROFILES[i];
            }
        }
    }
    
    /* Try ro.product.model */
    if (__system_property_get("ro.product.model", prop_value) > 0) {
        for (size_t i = 0; DEVICE_PROFILES[i].device_codename != NULL; i++) {
            if (strstr(prop_value, DEVICE_PROFILES[i].device_codename) != NULL) {
                LOG_I("Device matched by model: %s (%s)", 
                      DEVICE_PROFILES[i].device_name,
                      DEVICE_PROFILES[i].device_codename);
                return &DEVICE_PROFILES[i];
            }
        }
    }
    
    /* Try ro.product.vendor.device */
    if (__system_property_get("ro.product.vendor.device", prop_value) > 0) {
        for (size_t i = 0; DEVICE_PROFILES[i].device_codename != NULL; i++) {
            if (strcmp(prop_value, DEVICE_PROFILES[i].device_codename) == 0) {
                LOG_I("Device matched by vendor: %s (%s)", 
                      DEVICE_PROFILES[i].device_name,
                      DEVICE_PROFILES[i].device_codename);
                return &DEVICE_PROFILES[i];
            }
        }
    }
    
    /* Check manufacturer for Transsion brands */
    if (__system_property_get("ro.product.brand", prop_value) > 0) {
        if (strstr(prop_value, "TECNO") != NULL ||
            strstr(prop_value, "Infinix") != NULL ||
            strstr(prop_value, "itel") != NULL ||
            strstr(prop_value, "transsion") != NULL) {
            LOG_I("Detected Transsion brand: %s", prop_value);
        }
    }
    
    LOG_I("Using generic device profile");
    return &DEVICE_PROFILES[sizeof(DEVICE_PROFILES)/sizeof(DEVICE_PROFILES[0]) - 1];
}

/* ============================================================================
 * Brightness Calculation with Multiple Curves
 * ============================================================================ */

static float apply_gamma_curve(float value, float gamma) {
    return powf(value, 1.0f / gamma);
}

static float apply_log_curve(float value, float base) {
    if (value <= 0.0f) return 0.0f;
    return logf(value * (base - 1.0f) + 1.0f) / logf(base);
}

static float apply_exp_curve(float value, float factor) {
    return (expf(factor * value) - 1.0f) / (expf(factor) - 1.0f);
}

static float apply_s_curve(float value) {
    /* Smoothstep function for S-curve */
    return value * value * (3.0f - 2.0f * value);
}

static int calculate_brightness(float prop_val, int hw_min, int hw_max) {
    /* Validate hardware constraints */
    if (hw_min >= hw_max) {
        hw_min = 1;
        hw_max = g_state.profile->max_brightness;
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

    /* Normalize to 0.0-1.0 */
    float normalized = prop_val / BRIGHTNESS_MAX_VALUE;
    
    /* Apply selected brightness curve */
    float curve_output;
    switch (g_state.curve) {
        case CURVE_CUBE_ROOT:
            curve_output = cbrtf(normalized);
            break;
        case CURVE_LINEAR:
            curve_output = normalized;
            break;
        case CURVE_GAMMA_22:
            curve_output = apply_gamma_curve(normalized, 2.2f);
            break;
        case CURVE_GAMMA_24:
            curve_output = apply_gamma_curve(normalized, 2.4f);
            break;
        case CURVE_LOGARITHMIC:
            curve_output = apply_log_curve(normalized, 10.0f);
            break;
        case CURVE_EXPONENTIAL:
            curve_output = apply_exp_curve(normalized, 2.0f);
            break;
        case CURVE_S_CURVE:
            curve_output = apply_s_curve(normalized);
            break;
        case CURVE_CUSTOM:
        default:
            curve_output = cbrtf(normalized);
            break;
    }
    
    /* Map to hardware range */
    float mapped = curve_output * (float)(hw_max - hw_min) + (float)hw_min;
    int result = (int)roundf(mapped);
    
    /* Apply battery saver scaling */
    if (g_state.battery_saver_active && g_state.battery_saver_scale > 0.0f) {
        result = (int)((float)result * g_state.battery_saver_scale);
        LOG_D("Battery saver applied: scale=%.2f, result=%d", 
              g_state.battery_saver_scale, result);
    }
    
    /* Apply thermal throttling */
    if (g_state.thermal_throttle_active && g_state.thermal_scale > 0.0f) {
        result = (int)((float)result * g_state.thermal_scale);
        LOG_D("Thermal throttle applied: scale=%.2f, result=%d", 
              g_state.thermal_scale, result);
    }
    
    /* Clamp to hardware constraints */
    if (result < hw_min) result = hw_min;
    if (result > hw_max) result = hw_max;
    
    return result;
}

/* ============================================================================
 * Configuration File Parsing
 * ============================================================================ */

static void trim_whitespace(char* str) {
    if (!str) return;
    
    /* Trim leading */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return;
    
    /* Trim trailing */
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

static bool load_config_file(const char* path) {
    if (!file_exists(path)) {
        return false;
    }
    
    LOG_I("Loading config from: %s", path);
    
    FILE* f = fopen(path, "r");
    if (!f) {
        LOG_W("Failed to open config file: %s", path);
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        trim_whitespace(line);
        
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0') continue;
        
        char* eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char* key = line;
        char* value = eq + 1;
        trim_whitespace(key);
        trim_whitespace(value);
        
        /* Parse configuration options */
        if (strcmp(key, "curve") == 0) {
            if (strcmp(value, "cube_root") == 0) g_state.curve = CURVE_CUBE_ROOT;
            else if (strcmp(value, "linear") == 0) g_state.curve = CURVE_LINEAR;
            else if (strcmp(value, "gamma_22") == 0) g_state.curve = CURVE_GAMMA_22;
            else if (strcmp(value, "gamma_24") == 0) g_state.curve = CURVE_GAMMA_24;
            else if (strcmp(value, "logarithmic") == 0) g_state.curve = CURVE_LOGARITHMIC;
            else if (strcmp(value, "exponential") == 0) g_state.curve = CURVE_EXPONENTIAL;
            else if (strcmp(value, "s_curve") == 0) g_state.curve = CURVE_S_CURVE;
            else if (strcmp(value, "custom") == 0) g_state.curve = CURVE_CUSTOM;
        }
        else if (strcmp(key, "log_level") == 0) {
            g_state.log_level = atoi(value);
        }
        else if (strcmp(key, "battery_saver_scale") == 0) {
            g_state.battery_saver_scale = strtof(value, NULL);
        }
        else if (strcmp(key, "thermal_scale") == 0) {
            g_state.thermal_scale = strtof(value, NULL);
        }
        else if (strcmp(key, "wake_delay_ms") == 0) {
            /* Could override profile wake delay */
        }
    }
    
    fclose(f);
    LOG_I("Configuration loaded successfully");
    return true;
}

static void reload_configuration(void) {
    time_t now = time(NULL);
    if (now - g_state.last_config_reload < CONFIG_RELOAD_INTERVAL_SEC) {
        return;
    }
    
    g_state.last_config_reload = now;
    
    /* Try loading config from various locations */
    if (!load_config_file(CONFIG_PATH_USER)) {
        if (!load_config_file(CONFIG_PATH_PRIMARY)) {
            load_config_file(CONFIG_PATH_SECONDARY);
        }
    }
}

/* ============================================================================
 * State Management
 * ============================================================================ */

static void update_system_state(void) {
    /* Check battery saver status */
    g_state.battery_saver_active = get_bool_prop(BATTERY_SAVER_PROP, false);
    
    /* Check thermal throttling status */
    g_state.thermal_throttle_active = get_bool_prop(THERMAL_THROTTLE_PROP, false);
    
    /* Update screen state */
    g_state.screen_state = get_screen_state();
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

static bool initialize(void) {
    LOG_I("==============================================");
    LOG_I("  XTENSEI-OBright v%s '%s'", XTENSEI_VERSION_STRING, XTENSEI_CODENAME);
    LOG_I("  Universal OPlus Brightness Adaptor");
    LOG_I("  Copyright (C) 2026 XTENSEI");
    LOG_I("==============================================");
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    
    /* Initialize state */
    memset(&g_state, 0, sizeof(g_state));
    g_state.log_level = LOG_LEVEL_INFO;
    g_state.curve = CURVE_CUBE_ROOT;
    g_state.battery_saver_scale = 0.7f;  /* 70% in battery saver */
    g_state.thermal_scale = 0.8f;        /* 80% when thermal throttling */
    g_state.running = true;
    g_state.last_config_reload = 0;
    
    /* Detect device profile */
    g_state.profile = detect_device_profile();
    if (g_state.profile) {
        LOG_I("Device: %s (%s)", g_state.profile->device_name, 
              g_state.profile->device_codename ? g_state.profile->device_codename : "generic");
        LOG_I("Manufacturer: %s", g_state.profile->manufacturer);
        LOG_I("Default curve: %d", g_state.profile->default_curve);
        g_state.curve = g_state.profile->default_curve;
    }
    
    /* Detect backlight paths */
    if (!detect_backlight_paths()) {
        LOG_E("Failed to detect backlight paths!");
        return false;
    }
    
    /* Read hardware constraints */
    g_state.hw_max = g_state.profile->max_brightness;
    g_state.hw_min = g_state.profile->min_brightness;
    
    if (g_state.max_path) {
        int max = read_int_from_file(g_state.max_path, 0);
        if (max > 0) g_state.hw_max = max;
    }
    
    if (g_state.min_path) {
        int min = read_int_from_file(g_state.min_path, -1);
        if (min >= 0) g_state.hw_min = min;
    }
    
    LOG_I("Hardware constraints: Min=%d, Max=%d", g_state.hw_min, g_state.hw_max);
    
    /* Load configuration */
    reload_configuration();
    g_state.last_config_reload = 0;  /* Force reload on first check */
    
    /* Update system state */
    update_system_state();
    
    LOG_I("Initialization complete");
    return true;
}

/* ============================================================================
 * Main Service Entry Point
 * ============================================================================ */

int main(void) {
    /* Initialize */
    if (!initialize()) {
        LOG_E("Initialization failed, exiting");
        return EXIT_FAILURE;
    }
    
    /* Initialize state */
    float current_prop_val = get_float_prop(PROP_NAME, 0.0f);
    int prev_state = g_state.screen_state;

    int raw_initial = (current_prop_val == 0.0f) ? -1 : 
                      calculate_brightness(current_prop_val, g_state.hw_min, g_state.hw_max);
    int prev_bright = (raw_initial == -1) ? g_state.hw_min : raw_initial;
    g_state.last_written_value = -1;

    /* Apply initial state */
    if (prev_state != BRIGHTNESS_SCREEN_ON_STATE) {
        g_state.last_written_value = 0;
        write_backlight(0);
        LOG_I("Initial state: Screen OFF, brightness=0");
    } else {
        g_state.last_written_value = prev_bright;
        write_backlight(prev_bright);
        LOG_I("Initial state: Screen ON, brightness=%d", prev_bright);
    }

    /* Setup event trigger */
    uint32_t global_serial = 0;
    struct timespec no_wait = {0, 0};
    __system_property_wait(NULL, 0, &global_serial, &no_wait);

    LOG_I("Entering event-driven main loop (0%% CPU while idle)");
    LOG_I("Monitoring properties: %s, %s", PROP_NAME, STATE_PROP);

    /* Main event loop */
    while (g_state.running) {
        /* Check for signals */
        if (g_signal_received != 0) {
            LOG_I("Received signal %d, shutting down", g_signal_received);
            break;
        }
        
        /* Reload configuration periodically */
        reload_configuration();
        
        /* Update system state */
        update_system_state();

        /* Block until system property changes (0% CPU while waiting) */
        __system_property_wait(NULL, global_serial, &global_serial, NULL);

        float new_prop_val = get_float_prop(PROP_NAME, 0.0f);
        int cur_state = get_screen_state();

        /* Cache mechanism: Use last known brightness for 0.0f values */
        int raw_bright = (new_prop_val == 0.0f) ? -1 : 
                         calculate_brightness(new_prop_val, g_state.hw_min, g_state.hw_max);
        int cur_bright = (raw_bright == -1) ? prev_bright : raw_bright;
        if (cur_bright == -1) cur_bright = g_state.hw_min;

        /* Check if state or brightness changed */
        if (cur_bright != prev_bright || cur_state != prev_state) {
            int val_to_write = g_state.last_written_value;

            if (cur_state != BRIGHTNESS_SCREEN_ON_STATE) {
                /* Screen OFF/DOZE/AOD: Force 0 */
                val_to_write = 0;
                LOG_D("Screen state %d -> brightness=0", cur_state);
            } else {
                /* Screen ON */
                if (prev_state != BRIGHTNESS_SCREEN_ON_STATE) {
                    /* Hardware wake delay */
                    usleep(HARDWARE_WAKE_DELAY_US);
                    LOG_D("Screen wake -> brightness=%d", cur_bright);
                }
                val_to_write = cur_bright;
            }

            /* Write only if value changed */
            if (val_to_write != g_state.last_written_value) {
                if (write_backlight(val_to_write)) {
                    g_state.last_written_value = val_to_write;
                    LOG_D("Brightness updated: %d", val_to_write);
                }
            }
        }

        /* Update state tracking */
        prev_bright = cur_bright;
        prev_state = cur_state;
        current_prop_val = new_prop_val;
    }

    /* Cleanup */
    LOG_I("Shutting down XTENSEI-OBright");
    
    /* Set brightness to safe value before exit */
    if (g_state.screen_state == BRIGHTNESS_SCREEN_ON_STATE) {
        write_backlight(prev_bright);
    }
    
    return EXIT_SUCCESS;
}
