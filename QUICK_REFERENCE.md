# Quick Reference Guide

## XTENSEI-OBright v2.0.0 "Phoenix"

---

## ⚡ Quick Install

### Via Recovery
```
1. Download flashable ZIP
2. Boot to TWRP/OrangeFox
3. Flash ZIP
4. Reboot
```

### Via ADB
```bash
cd dist/adb-installer
./install.sh
```

---

## 🔍 Check If It's Working

```bash
# View logs
adb logcat | grep XTENSEI-OBright

# Check if service is running
adb shell ps | grep yamada.obright

# Check brightness path
adb shell ls -l /sys/class/leds/lcd-backlight/brightness
```

---

## ⚙️ Configuration Quick Reference

Edit `/vendor/etc/xtensei-obright.conf`:

```ini
# For IPS displays (most TECNO, Infinix)
curve = cube_root

# For AMOLED displays (Phantom, Zero, GT)
curve = gamma_22

# For very dark rooms
curve = gamma_24

# Log level (0-4)
log_level = 2

# Battery saver (70% brightness)
battery_saver_scale = 0.7

# Thermal throttle (80% brightness)
thermal_scale = 0.8
```

---

## 🎯 Brightness Curves

| Display Type | Recommended Curve |
|--------------|-------------------|
| IPS (Spark, Hot, Pova) | `cube_root` |
| AMOLED (Phantom, Zero) | `gamma_22` |
| AMOLED (dark room) | `gamma_24` |
| Basic LCD (itel) | `linear` |
| Gaming (GT series) | `s_curve` |

---

## 🐛 Troubleshooting

### Service not starting
```bash
adb logcat | grep XTENSEI-OBright
adb shell su -c "/vendor/bin/hw/yamada.obright-V1@3.0-service"
```

### Brightness not changing
```bash
adb shell su -c "echo 1000 > /sys/class/leds/lcd-backlight/brightness"
```

### Restart service
```bash
adb shell su -c "setprop vendor.xtensei.obright.restart 1"
```

### Uninstall
```bash
cd dist/adb-installer
./uninstall.sh
```

---

## 📱 Device Codenames

Find your codename:
```bash
adb shell getprop ro.product.device
```

### Common Codenames

| Brand | Pattern | Examples |
|-------|---------|----------|
| TECNO | LH*, LJ*, LK*, LM* | LH8n, LJ9, LK1 |
| Infinix | X6* | X6833, X6834 |
| iTel | A*, S*, P* | A70, S23 |

---

## 🔨 Build from Source

```bash
cd Source
chmod +x build.sh
./build.sh
```

Requirements:
- Android NDK r28+
- `ANDROID_NDK_HOME` set

---

## 📊 File Locations

| File | Path |
|------|------|
| Binary | `/vendor/bin/hw/yamada.obright-V1@3.0-service` |
| Init Script | `/vendor/etc/init/init.yamada.obright.rc` |
| Config | `/vendor/etc/xtensei-obright.conf` |
| Backup | `/data/xtensei-obright-backup/` |

---

## 💡 Tips

1. **Always backup** before installing
2. **Test different curves** for your display
3. **Lower battery_saver_scale** for better battery life
4. **Increase log_level** for debugging
5. **Check thermal_scale** if device gets hot

---

## 📞 Support

- GitHub: https://github.com/XTENSEI/XTENSEI-OBright
- Issues: https://github.com/XTENSEI/XTENSEI-OBright/issues

---

**XTENSEI-OBright v2.0.0** - Universal brightness control for Transsion devices
