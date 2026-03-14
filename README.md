# XTENSEI-OBright

**OPlus Brightness Adaptor for Transsion Devices**

A specialized brightness control daemon designed to fix screen dimming issues on OPlus-based custom ROMs running on Transsion devices (TECNO, Infinix, iTel) with IPS displays.

[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Android%2011%2B-green.svg)]()
[![Architecture](https://img.shields.io/badge/arch-arm64--v8a-red.svg)]()

## Features

- ✅ **Fixes sudden screen dimming** on IPS displays
- ✅ **Event-driven architecture** - 0% CPU usage while idle
- ✅ **Cube root brightness mapping** for natural perceptual control
- ✅ **Hardware constraint awareness** - respects min/max brightness limits
- ✅ **Automatic fallback** to alternative backlight paths
- ✅ **Screen state management** - proper handling of OFF/DOZE/AOD states
- ✅ **Optimized for Transsion devices** - TECNO, Infinix, iTel

## Supported Devices

This brightness adaptor is specifically designed for **Transsion devices** with **OPlus-based custom ROMs**:

- TECNO LH8n (and similar models)
- Infinix devices with IPS displays
- iTel devices with brightness dimming issues

> **Note:** If your device experiences sudden brightness dimming on IPS displays with OPlus ROMs, this adaptor should help.

## Installation

### Prerequisites

- Rooted Android device
- Custom recovery (TWRP, OrangeFox, etc.)
- Android 11 (API 30) or higher
- NDK for building (if compiling from source)

### Method 1: Pre-built Binary

1. Download the pre-built binary from releases
2. Push to your device:
   ```bash
   adb push out/vendor /vendor
   ```

3. Set proper permissions:
   ```bash
   adb shell
   su
   chmod 0755 /vendor/bin/hw/yamada.obright-V1@3.0-service
   chmod 0644 /vendor/etc/init/init.yamada.obright.rc
   ```

4. Add SELinux context to `file_contexts`:
   ```
   /vendor/bin/hw/yamada\.obright-V1@3\.0-service    u:object_r:mtk_hal_light_exec:s0
   ```

5. Reboot your device

### Method 2: Build from Source

#### Requirements

- Android NDK (r28 or later recommended)
- Linux build environment
- `ANDROID_NDK_HOME` environment variable set

#### Build Steps

```bash
cd Source
chmod +x build.sh
./build.sh
```

The build script will:
1. Compile the native binary for arm64-v8a
2. Generate the init RC file
3. Output files to `out/vendor/`

#### Deploy to Device

```bash
# Push binary and init script
adb push out/vendor/bin/hw/yamada.obright-V1@3.0-service /vendor/bin/hw/
adb push out/vendor/etc/init/init.yamada.obright.rc /vendor/etc/init/

# Set permissions
adb shell su -c "chmod 0755 /vendor/bin/hw/yamada.obright-V1@3.0-service"
adb shell su -c "chmod 0644 /vendor/etc/init/init.yamada.obright.rc"

# Reboot
adb reboot
```

## Configuration

### SELinux Context

Add the following to your `file_contexts` file:

```
/vendor/bin/hw/yamada\.obright-V1@3\.0-service    u:object_r:mtk_hal_light_exec:s0
```

### System Properties

The daemon monitors these system properties:

| Property | Description | Values |
|----------|-------------|--------|
| `debug.tracing.screen_brightness` | Brightness level | 0.0 - 1.0 (slider) or 222 - 8191 (raw) |
| `debug.tracing.screen_state` | Screen power state | 0=OFF, 1=DOZE, 2=ON, 3=AOD |

## How It Works

### Brightness Mapping

The adaptor uses a **cube root mapping** algorithm for perceptual brightness linearity:

1. **Settings Slider (0.0-1.0)**: Mapped to hardware range (222-8191)
2. **Cube Root Curve**: Applied for natural brightness perception
3. **Hardware Constraints**: Final value clamped to device min/max

```
normalized = brightness / 8191
linear = cbrt(normalized)
output = linear * max_brightness
```

### Event-Driven Design

Unlike polling-based solutions, XTENSEI-OBright:
- Uses `__system_property_wait()` for **zero CPU usage** while idle
- Instantly responds to brightness changes
- No battery impact

### Screen State Handling

| State | Action |
|-------|--------|
| OFF (0) | Set brightness to 0 |
| DOZE (1) | Set brightness to 0 |
| ON (2) | Apply calculated brightness |
| AOD (3) | Set brightness to 0 |

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│              Android Framework Layer                    │
│  (Settings, Display Manager, Property Service)          │
└────────────────────┬────────────────────────────────────┘
                     │ Property Changes
                     ▼
┌─────────────────────────────────────────────────────────┐
│           XTENSEI-OBright Daemon                        │
│  ┌───────────────────────────────────────────────────┐  │
│  │  Event Loop (0% CPU while waiting)                │  │
│  │  - Monitors system properties                     │  │
│  │  - Calculates brightness (cube root mapping)      │  │
│  │  - Manages screen state transitions               │  │
│  └───────────────────────────────────────────────────┘  │
└────────────────────┬────────────────────────────────────┘
                     │ Write
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Kernel Backlight Driver                    │
│  /sys/class/leds/lcd-backlight/brightness               │
└─────────────────────────────────────────────────────────┘
```

## Troubleshooting

### Service Not Starting

1. Check logs:
   ```bash
   adb logcat | grep XTENSEI-OBright
   ```

2. Verify permissions:
   ```bash
   adb shell ls -l /vendor/bin/hw/yamada.obright-V1@3.0-service
   ```

3. Check SELinux context:
   ```bash
   adb shell ls -Z /vendor/bin/hw/yamada.obright-V1@3.0-service
   ```

### Brightness Not Changing

1. Verify backlight path exists:
   ```bash
   adb shell ls -l /sys/class/leds/lcd-backlight/
   ```

2. Check if alternative path is needed:
   ```bash
   adb shell ls -l /sys/class/backlight/
   ```

3. Test manual write:
   ```bash
   adb shell su -c "echo 1000 > /sys/class/leds/lcd-backlight/brightness"
   ```

## Building for Different Devices

Edit `Source/build.sh` to customize:

```bash
# Target API level (minimum Android 11)
API_LEVEL=30

# Output binary name
BIN_NAME="yamada.obright-V1@3.0-service"

# NDK path
NDK_ROOT=${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/28.2.13676358}
```

## Credits

- **Original Concept**: [@ryanistr](https://github.com/ryanistr) - OPlus-Brightness-Adaptor-For-Transsion
- **XTENSEI Version**: Enhanced with better error handling, alternative path support, and improved documentation

## Support the Project

If this fix helps your device, consider supporting:

- **Global**: [Sociabuzz](https://sociabuzz.com/kanagawa_yamada/tribe)
- **Indonesia (QRIS)**: [Telegram](https://t.me/KLAGen2/86)

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0).

See [LICENSE](LICENSE) for the full license text.

### Summary

- ✅ Free to use and modify
- ✅ Must share source code if distributed
- ✅ Same license for derivatives
- ❌ No warranty provided

## Disclaimer

⚠️ **Use at your own risk.** Modifying system files and backlight control can potentially cause issues. Always backup your device before making system modifications.

The authors are not responsible for any damage to your device.

---

**XTENSEI-OBright** - Bringing proper brightness control to Transsion devices
