# XTENSEI-OBright

**Universal OPlus Brightness Adaptor for Transsion Devices**

[![Version](https://img.shields.io/badge/version-2.0.0--universal-blue.svg)]()
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Android%2011%2B-green.svg)]()
[![Architecture](https://img.shields.io/badge/arch-arm64--v8a-red.svg)]()
[![Codename](https://img.shields.io/badge/codename-Phoenix-purple.svg)]()

---

## 🌟 Features

### Core Features
- ✅ **Universal Backlight Support** - 20+ backlight paths for maximum device compatibility
- ✅ **Device Detection** - Automatic device profile detection for TECNO, Infinix, iTel
- ✅ **Advanced Brightness Curves** - 7 different curves (Cube Root, Gamma, Log, Exp, S-Curve, Linear)
- ✅ **Zero CPU Usage** - Event-driven architecture with property wait
- ✅ **Battery Saver Integration** - Automatic brightness reduction in power saving mode
- ✅ **Thermal Throttling** - Automatic brightness reduction when device overheats
- ✅ **Configuration File Support** - Runtime customization without recompilation
- ✅ **Comprehensive Logging** - 5 log levels for easy debugging
- ✅ **Signal Handling** - Graceful shutdown on SIGINT, SIGTERM, SIGHUP
- ✅ **Fallback Paths** - Automatic fallback to alternative backlight paths

### New in v2.0.0 "Phoenix"
- 🔥 Universal backlight path detection (20+ paths)
- 🔥 Device-specific brightness profiles (18+ devices)
- 🔥 Multiple brightness curve algorithms
- 🔥 Configuration file support
- 🔥 Battery saver integration
- 🔥 Thermal throttling support
- 🔥 Flashable ZIP package
- 🔥 ADB installer script
- 🔥 SELinux policy files
- 🔥 Comprehensive documentation

---
### Generic Support

Any Transsion device with OPlus-based ROM should work with the generic profile.

---

## 🔧 Installation

### Method 1: Flashable ZIP (Recommended)

1. Download `XTENSEI-OBright-2.0.0-universal-flashable.zip`
2. Boot to custom recovery (TWRP, OrangeFox)
3. Flash the ZIP
4. Reboot

**That's it!** The service starts automatically on boot.

### Method 2: ADB Installer

```bash
cd dist/adb-installer
./install.sh
```

### Method 3: Manual Installation

```bash
# Push files
adb push out/vendor/bin/hw/yamada.obright-V1@3.0-service /vendor/bin/hw/
adb push out/vendor/etc/init/init.yamada.obright.rc /vendor/etc/init/
adb push out/vendor/etc/xtensei-obright.conf /vendor/etc/

# Set permissions
adb shell su -c "chmod 0755 /vendor/bin/hw/yamada.obright-V1@3.0-service"
adb shell su -c "chmod 0644 /vendor/etc/init/init.yamada.obright.rc"
adb shell su -c "chmod 0644 /vendor/etc/xtensei-obright.conf"

# Reboot
adb reboot
```

### Method 4: Build from Source

```bash
cd Source
chmod +x build.sh
./build.sh
```

---

## ⚙️ Configuration

### Configuration File

Edit `/vendor/etc/xtensei-obright.conf`:

```ini
# Brightness curve (cube_root, linear, gamma_22, gamma_24, logarithmic, exponential, s_curve)
curve = cube_root

# Log level (0=ERROR, 1=WARN, 2=INFO, 3=DEBUG, 4=VERBOSE)
log_level = 2

# Battery saver scale (0.0 - 1.0)
battery_saver_scale = 0.7

# Thermal throttle scale (0.0 - 1.0)
thermal_scale = 0.8

# Screen wake delay (milliseconds)
wake_delay_ms = 100
```

### Brightness Curves Explained

| Curve | Description | Best For |
|-------|-------------|----------|
| `cube_root` | Perceptual linearity | Most IPS displays (default) |
| `linear` | Direct 1:1 mapping | Basic displays |
| `gamma_22` | Gamma 2.2 correction | AMOLED displays |
| `gamma_24` | Gamma 2.4 correction | Dark room usage |
| `logarithmic` | Log curve | Very dark environments |
| `exponential` | Exponential curve | Bright environments |
| `s_curve` | S-curve response | Balanced response |

---

## 📊 Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Android Framework Layer                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ │
│  │   Settings  │  │   Display   │  │    Property Service     │ │
│  │   Slider    │  │   Manager   │  │  (brightness, state)    │ │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────────┘
                          │ Property Changes
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                  XTENSEI-OBright Daemon v2.0.0                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Event Loop (0% CPU while waiting)                        │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │  │
│  │  │   Device     │  │   Backlight  │  │   Config     │    │  │
│  │  │   Detection  │  │   Detection  │  │   Loader     │    │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘    │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │  │
│  │  │  Brightness  │  │   Battery    │  │   Thermal    │    │  │
│  │  │   Curves     │  │   Saver      │  │  Throttling  │    │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘    │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────┬───────────────────────────────────────┘
                          │ Write
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Kernel Backlight Driver                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  /sys/class/leds/lcd-backlight/brightness               │   │
│  │  /sys/class/backlight/panel0-backlight/brightness       │   │
│  │  (20+ supported paths with auto-detection)              │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔍 Troubleshooting

### Service Not Starting

```bash
# Check logs
adb logcat | grep XTENSEI-OBright

# Check if binary exists
adb shell ls -l /vendor/bin/hw/yamada.obright-V1@3.0-service

# Check permissions
adb shell ls -Z /vendor/bin/hw/yamada.obright-V1@3.0-service

# Manual start for testing
adb shell su -c "/vendor/bin/hw/yamada.obright-V1@3.0-service"
```

### Brightness Not Changing

```bash
# Check backlight path
adb shell ls -l /sys/class/leds/lcd-backlight/
adb shell ls -l /sys/class/backlight/

# Test manual write
adb shell su -c "echo 1000 > /sys/class/leds/lcd-backlight/brightness"

# Check if path is writable
adb shell su -c "touch /sys/class/leds/lcd-backlight/brightness"
```

### Wrong Brightness Curve

Edit `/vendor/etc/xtensei-obright.conf`:

```ini
# For IPS displays
curve = cube_root

# For AMOLED displays
curve = gamma_22
```

Then restart:
```bash
adb shell su -c "setprop vendor.xtensei.obright.restart 1"
```

### Battery Saver Not Working

Check battery saver property:
```bash
adb shell getprop sys.power.saving
```

### Thermal Throttling Not Working

Check thermal property:
```bash
adb shell getprop sys.thermal.throttle
```

---

## 🛠️ Building

### Requirements

- Android NDK r28 or later
- Linux build environment (Ubuntu, Debian, Fedora, etc.)
- `ANDROID_NDK_HOME` environment variable set

### Build Commands

```bash
cd Source
chmod +x build.sh
./build.sh
```

### Build Output

```
out/
├── vendor/
│   ├── bin/
│   │   └── hw/
│   │       └── yamada.obright-V1@3.0-service  (binary)
│   ├── etc/
│   │   ├── init/
│   │   │   └── init.yamada.obright.rc         (init script)
│   │   └── xtensei-obright.conf               (config template)
dist/
├── XTENSEI-OBright-2.0.0-universal-flashable.zip
└── adb-installer/
    ├── install.sh
    ├── uninstall.sh
    └── vendor/
```

---

## 📝 File Descriptions

| File | Description |
|------|-------------|
| `Main.c` | Main daemon source code |
| `build.sh` | Build script |
| `xtensei-obright.conf` | Configuration template |
| `xtensei_obright.te` | SELinux policy |
| `init.yamada.obright.rc` | Init script |
| `update-binary` | Flashable ZIP installer |
| `updater-script` | Recovery updater script |

---

## 🔐 SELinux Policy

For devices with enforcing SELinux, add to your `file_contexts`:

```
/vendor/bin/hw/yamada\.obright-V1@3\.0-service    u:object_r:xtensei_obright_exec:s0
```

Compile the provided `xtensei_obright.te` with your device's SELinux policy.

---

## 📈 Performance

| Metric | Value |
|--------|-------|
| Binary Size | ~20 KB (stripped) |
| RAM Usage | < 1 MB |
| CPU Usage | 0% (idle), < 0.1% (active) |
| Battery Impact | Negligible |
| Boot Time | < 100ms |

---

## 🤝 Contributing

### Adding Device Profiles

Edit `Main.c` and add to `DEVICE_PROFILES[]`:

```c
{"device-codename", "Device Name", "Manufacturer", max_brightness, min_brightness, curve, wake_delay, thermal, battery}
```

### Adding Backlight Paths

Add to `BACKLIGHT_PATHS[]` array in `Main.c`.

### Testing

1. Build the binary
2. Test on your device
3. Report results on GitHub

---

## 📞 Support

- **GitHub**: https://github.com/XTENSEI/XTENSEI-OBright
- **Issues**: https://github.com/XTENSEI/XTENSEI-OBright/issues

---

## 💖 Support the Project

If this fix helps your device:

- **Global**: [Sociabuzz](https://sociabuzz.com/kanagawa_yamada/tribe)
- **Indonesia (QRIS)**: [Telegram](https://t.me/KLAGen2/86)

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0.

See [LICENSE](LICENSE) for the full license text.

### Summary

- ✅ Free to use and modify
- ✅ Must share source code if distributed
- ✅ Same license for derivatives
- ❌ No warranty provided

---

## ⚠️ Disclaimer

**Use at your own risk.** Modifying system files and backlight control can potentially cause issues. Always backup your device before making system modifications.

The authors are not responsible for any damage to your device.

---

## 📜 Credits

- **Original Concept**: [@ryanistr](https://github.com/ryanistr) - OPlus-Brightness-Adaptor-For-Transsion
- **XTENSEI v2.0.0**: Universal rewrite with device detection, multiple curves, and comprehensive support
- **Testing**: TECNO, Infinix, iTel device community

---

**XTENSEI-OBright v2.0.0 "Phoenix"** - The ultimate brightness solution for Transsion devices
