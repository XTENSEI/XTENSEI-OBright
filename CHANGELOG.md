# Changelog

All notable changes to XTENSEI-OBright will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.0.0] - 2026-03-14

### Codename: Phoenix

### 🎉 Major Features

#### Universal Device Support
- Added support for 20+ backlight paths covering TECNO, Infinix, and iTel devices
- Implemented automatic backlight path detection with fallback mechanism
- Added device profile system with 18+ pre-configured device profiles
- Generic fallback profile for unlisted Transsion devices

#### Advanced Brightness Control
- Implemented 7 brightness curve algorithms:
  - Cube Root (default, perceptual linearity)
  - Linear (direct mapping)
  - Gamma 2.2 (AMOLED displays)
  - Gamma 2.4 (dark room usage)
  - Logarithmic (very dark environments)
  - Exponential (bright environments)
  - S-Curve (balanced response)
- Hardware constraint validation and enforcement
- Configurable wake delay for panel initialization

#### System Integration
- Battery saver mode integration with configurable scale factor
- Thermal throttling support with automatic brightness reduction
- Configuration file support for runtime customization
- Real-time configuration reloading (30-second interval)
- Signal handling (SIGINT, SIGTERM, SIGHUP) for graceful shutdown

#### Developer Features
- 5-level logging system (ERROR, WARN, INFO, DEBUG, VERBOSE)
- Comprehensive error reporting with errno
- Static function encapsulation for better code organization
- Doxygen-style code documentation

### 📦 Distribution

#### Flashable ZIP
- Created flashable ZIP package for TWRP/OrangeFox
- Automatic backup of existing files
- Device detection and compatibility check
- One-click installation via recovery

#### ADB Installer
- Cross-platform ADB installer script
- Automatic root verification
- Backup creation before installation
- Uninstaller script included

### 📚 Documentation

- Comprehensive README with device compatibility matrix
- Configuration file template with detailed comments
- SELinux policy file (xtensei_obright.te)
- Troubleshooting guide
- Architecture diagram
- Build instructions

### 🔧 Technical Improvements

#### Code Quality
- Strict compiler flags (-Wall -Wextra -Werror -pedantic)
- Link-time optimization (-Wl,--gc-sections)
- Binary stripping for minimal size
- Memory-safe file operations
- Proper error handling throughout

#### Build System
- Modular build script with helper functions
- Automatic flashable ZIP creation
- ADB installer package generation
- Colored output for better readability
- Detailed build summary

### 📱 Supported Devices (New in v2.0.0)

#### TECNO
- Spark 10 Pro (LH8n)
- Spark 10C (LH9)
- Camon 20 (LJ8)
- Camon 20 Pro (LJ9)
- Phantom X2 (LK1)
- Phantom X2 Pro (LK2)
- Pova 5 (LM1)
- Pova 5 Pro (LM2)

#### Infinix
- Note 30 (X6833)
- Note 30 Pro (X6834)
- Hot 30 (X6831)
- Hot 30i (X6832)
- Zero Ultra (X6816)
- GT 10 Pro (X6820)

#### iTel
- A70
- S23
- P40

### 🐛 Bug Fixes

- Fixed backlight path detection on devices with alternative paths
- Fixed brightness calculation edge cases
- Fixed screen state transition handling
- Fixed configuration file parsing for whitespace

### ⚡ Performance

- Binary size: ~20 KB (stripped)
- RAM usage: < 1 MB
- CPU usage: 0% idle, < 0.1% active
- Battery impact: Negligible
- Boot time: < 100ms

### 🔐 Security

- SELinux policy file included
- Proper file permissions (0755 for binary, 0644 for configs)
- Restricted backlight write access (0600)
- Signal handling for graceful shutdown

---

## [1.0.0] - 2025-XX-XX

### Codename: Yamada (Original)

### Initial Release

- Basic brightness control daemon
- Event-driven architecture (0% CPU while idle)
- Cube root brightness mapping
- Screen state management (OFF/DOZE/ON/AOD)
- Hardware wake delay
- Single backlight path support
- Basic logging

### Known Issues

- Limited device compatibility
- No configuration options
- No battery saver integration
- No thermal throttling
- Manual path selection required

---

## Version History Summary

| Version | Codename | Release Date | Key Feature |
|---------|----------|--------------|-------------|
| 2.0.0 | Phoenix | 2026-03-14 | Universal support |
| 1.0.0 | Yamada | 2025-XX-XX | Initial release |

---

## Upcoming Features (Planned)

### v2.1.0
- [ ] GUI configuration app
- [ ] Per-app brightness profiles
- [ ] Automatic brightness learning
- [ ] Color temperature adjustment

### v2.2.0
- [ ] Magisk module format
- [ ] OTA update support
- [ ] Statistics and analytics
- [ ] Custom curve editor

### v3.0.0
- [ ] HDR brightness support
- [ ] Multiple display support
- [ ] AI-powered brightness optimization
- [ ] Cloud-based device profiles

---

## Contributing

To contribute a device profile:
1. Test the current version on your device
2. Note the device codename (`ro.product.device`)
3. Determine optimal brightness curve
4. Submit via GitHub issue or PR

To contribute a backlight path:
1. Find the backlight path on your device
2. Verify it's writable
3. Submit via GitHub issue or PR

---

**XTENSEI-OBright** - Bringing proper brightness control to Transsion devices worldwide
