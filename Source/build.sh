#!/bin/bash
#
# XTENSEI-OBright Build Script
# Universal OPlus Brightness Adaptor for Transsion Devices
#
# Copyright (C) 2026 XTENSEI <andriegalutera@gmail.com>
# Licensed under GPL-3.0
#
# This script builds the XTENSEI-OBright binary and creates
# distribution packages including flashable ZIP for recovery.
#

set -e  # Exit on error

# =============================================================================
# Configuration
# =============================================================================

# NDK Path - Check environment variable first, then default
NDK_ROOT="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/28.2.13676358}"

# Build Configuration
API_LEVEL=30                          # Android 11 (safe baseline)
TARGET="aarch64-linux-android${API_LEVEL}"
BUILD_TYPE="release"
VERSION="2.0.0"
CODENAME="universal"

# Output Configuration
BIN_NAME="yamada.obright-V1@3.0-service"
RC_NAME="init.yamada.obright.rc"
CONFIG_NAME="xtensei-obright.conf"
OUT_DIR="out"
BIN_DIR="${OUT_DIR}/vendor/bin/hw"
ETC_DIR="${OUT_DIR}/vendor/etc/init"
DIST_DIR="dist"

# Compiler Flags
CFLAGS="-O3 -Wall -Wextra -Werror -pedantic -ffunction-sections -fdata-sections"
LDFLAGS="-llog -lm -Wl,--gc-sections -s"

# =============================================================================
# Helper Functions
# =============================================================================

log_info() {
    echo -e "\033[0;34m[INFO]\033[0m $1"
}

log_success() {
    echo -e "\033[0;32m[SUCCESS]\033[0m $1"
}

log_error() {
    echo -e "\033[0;31m[ERROR]\033[0m $1" >&2
}

log_warn() {
    echo -e "\033[0;33m[WARN]\033[0m $1" >&2
}

log_header() {
    echo -e "\n\033[1;36m==============================================\033[0m"
    echo -e "\033[1;36m$1\033[0m"
    echo -e "\033[1;36m==============================================\033[0m\n"
}

cleanup() {
    if [ -d "${OUT_DIR}" ]; then
        log_info "Cleaning previous build..."
        rm -rf "${OUT_DIR}"
    fi
}

check_dependencies() {
    # Check NDK
    if [ ! -d "${NDK_ROOT}" ]; then
        log_error "NDK not found at: ${NDK_ROOT}"
        log_error ""
        log_error "Please set ANDROID_NDK_HOME environment variable:"
        log_error "  export ANDROID_NDK_HOME=/path/to/ndk"
        log_error ""
        log_error "Or edit this script to set the correct NDK path."
        exit 1
    fi

    # Check toolchain
    local TOOLCHAIN="${NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/bin"
    local CC="${TOOLCHAIN}/clang"

    if [ ! -f "${CC}" ]; then
        log_error "Clang compiler not found at: ${CC}"
        log_error "Please verify your NDK installation."
        exit 1
    fi

    # Check source file
    if [ ! -f "Main.c" ]; then
        log_error "Source file 'Main.c' not found in current directory."
        exit 1
    fi

    echo "${TOOLCHAIN}"
}

generate_rc_file() {
    log_info "Generating init RC file..."

    cat > "${ETC_DIR}/${RC_NAME}" <<EOF
# XTENSEI-OBright Init Script
# Universal OPlus Brightness Adaptor for Transsion Devices
#
# Copyright (C) 2026 XTENSEI <andriegalutera@gmail.com>
# Licensed under GPL-3.0
#
# Version: ${VERSION}-${CODENAME}

service xtensei_obright /vendor/bin/hw/${BIN_NAME}
    class hal
    user system
    group system graphics
    capabilities SYS_NICE SYS_TIME
    writepid /dev/cpuset/system-background/tasks
    ioprio rt 0
    task_profiles HighEnergySaving

    # Restart on failure
    oneshot
    restart_delay 1000

on property:sys.boot_completed=1
    # Wait for system to fully boot
    wait /sys/class/leds 5

    # Set ownership to system user
    chown system system /sys/class/leds/lcd-backlight/brightness
    chown system system /sys/class/backlight/panel0-backlight/brightness

    # Restrict write access to owner only (security)
    chmod 0600 /sys/class/leds/lcd-backlight/brightness
    chmod 0600 /sys/class/backlight/panel0-backlight/brightness

    # Start the brightness service
    start xtensei_obright

on property:vendor.xtensei.obright.restart=1
    # Manual restart trigger
    start xtensei_obright

on property:sys.powerctl=shutdown
    # Graceful shutdown
    stop xtensei_obright
EOF

    log_success "Created RC file: ${ETC_DIR}/${RC_NAME}"
}

build_binary() {
    local TOOLCHAIN="$1"
    local CC="${TOOLCHAIN}/clang"

    log_info "Building ${BIN_NAME}..."
    log_info "  Target: ${TARGET}"
    log_info "  API Level: ${API_LEVEL}"
    log_info "  Optimization: -O3"
    log_info "  Version: ${VERSION}-${CODENAME}"

    # Compile
    "${CC}" \
        --target="${TARGET}" \
        ${CFLAGS} \
        -o "${BIN_DIR}/${BIN_NAME}" \
        Main.c \
        ${LDFLAGS}

    if [ $? -eq 0 ]; then
        log_success "Binary built: ${BIN_DIR}/${BIN_NAME}"
        
        # Show binary info
        local SIZE=$(ls -lh "${BIN_DIR}/${BIN_NAME}" | awk '{print $5}')
        local STRIPPED_SIZE=$(size "${BIN_DIR}/${BIN_NAME}" 2>/dev/null | tail -1 | awk '{print $1+$2+$3}' || echo "N/A")
        log_info "  Size: ${SIZE}"
        log_info "  Stripped size: ${STRIPPED_SIZE} bytes"
    else
        log_error "Build failed!"
        exit 1
    fi
}

set_permissions() {
    log_info "Setting file permissions..."
    chmod 0755 "${BIN_DIR}/${BIN_NAME}"
    chmod 0644 "${ETC_DIR}/${RC_NAME}"
    
    # Copy config file
    if [ -f "${CONFIG_NAME}" ]; then
        cp "${CONFIG_NAME}" "${OUT_DIR}/vendor/etc/"
        chmod 0644 "${OUT_DIR}/vendor/etc/${CONFIG_NAME}"
        log_success "Config file copied"
    fi
    
    log_success "Permissions set"
}

create_flashable_zip() {
    log_header "Creating Flashable ZIP"
    
    local ZIP_NAME="XTENSEI-OBright-${VERSION}-${CODENAME}-flashable.zip"
    local ZIP_DIR="${DIST_DIR}/flashable"
    
    mkdir -p "${ZIP_DIR}"
    
    # Copy vendor files
    cp -r "${OUT_DIR}/vendor" "${ZIP_DIR}/"
    
    # Copy META-INF
    if [ -d "META-INF" ]; then
        cp -r "META-INF" "${ZIP_DIR}/"
    else
        log_warn "META-INF directory not found, creating basic one..."
        mkdir -p "${ZIP_DIR}/META-INF/com/google/android"
        
        # Create minimal update-binary
        cat > "${ZIP_DIR}/META-INF/com/google/android/update-binary" << 'UPDATER'
#!/sbin/sh
OUTFD=$2
ZIPFILE=$3
ui_print() { echo "ui_print $1" > $OUTFD; echo "ui_print" > $OUTFD; }
ui_print " "
ui_print "XTENSEI-OBright v2.0.0-universal"
ui_print "Installing..."
mount /vendor 2>/dev/null
mkdir -p /vendor/bin/hw /vendor/etc/init /vendor/etc
unzip -o "$ZIPFILE" 'vendor/*' -d /
chmod 0755 /vendor/bin/hw/yamada.obright-V1@3.0-service
chmod 0644 /vendor/etc/init/init.yamada.obright.rc
chmod 0644 /vendor/etc/xtensei-obright.conf 2>/dev/null
umount /vendor 2>/dev/null
ui_print "Installation Complete!"
ui_print "Reboot your device."
exit 0
UPDATER
        chmod +x "${ZIP_DIR}/META-INF/com/google/android/update-binary"
        
        # Create updater-script
        echo "# XTENSEI-OBright Flashable ZIP" > "${ZIP_DIR}/META-INF/com/google/android/updater-script"
    fi
    
    # Create ZIP
    cd "${ZIP_DIR}"
    zip -r9 "../../${ZIP_NAME}" * .[^.]* 2>/dev/null || zip -r9 "../../${ZIP_NAME}" *
    cd - > /dev/null
    
    log_success "Created: ${DIST_DIR}/${ZIP_NAME}"
    
    # Show ZIP info
    local ZIP_SIZE=$(ls -lh "${DIST_DIR}/${ZIP_NAME}" | awk '{print $5}')
    log_info "  ZIP Size: ${ZIP_SIZE}"
}

create_adb_installer() {
    log_header "Creating ADB Installer Package"
    
    local INSTALL_DIR="${DIST_DIR}/adb-installer"
    mkdir -p "${INSTALL_DIR}"
    
    # Copy vendor files
    cp -r "${OUT_DIR}/vendor" "${INSTALL_DIR}/"
    
    # Create installer script
    cat > "${INSTALL_DIR}/install.sh" << 'INSTALLER'
#!/bin/bash
# XTENSEI-OBright ADB Installer
# Copyright (C) 2026 XTENSEI

echo "=============================================="
echo "  XTENSEI-OBright ADB Installer"
echo "=============================================="
echo ""

# Check ADB
if ! command -v adb &> /dev/null; then
    echo "ERROR: adb not found in PATH"
    exit 1
fi

# Check device
if ! adb get-state &> /dev/null; then
    echo "ERROR: No device connected or not authorized"
    echo "Run 'adb devices' to check"
    exit 1
fi

echo "Device found: $(adb shell getprop ro.product.device)"
echo "Brand: $(adb shell getprop ro.product.brand)"
echo ""

# Check root
if ! adb shell su -c "id" &> /dev/null; then
    echo "ERROR: Device not rooted or su not available"
    exit 1
fi

echo "Root access confirmed"
echo ""

# Backup
echo "Creating backup..."
adb shell su -c "mkdir -p /data/xtensei-obright-backup"
adb shell su -c "cp /vendor/bin/hw/yamada.obright-V1@3.0-service /data/xtensei-obright-backup/ 2>/dev/null || true"
adb shell su -c "cp /vendor/etc/init/init.yamada.obright.rc /data/xtensei-obright-backup/ 2>/dev/null || true"
echo "Backup created at /data/xtensei-obright-backup"
echo ""

# Install
echo "Installing files..."
adb shell su -c "mkdir -p /vendor/bin/hw /vendor/etc/init /vendor/etc"
adb push vendor/bin/hw/yamada.obright-V1@3.0-service /vendor/bin/hw/
adb push vendor/etc/init/init.yamada.obright.rc /vendor/etc/init/
if [ -f vendor/etc/xtensei-obright.conf ]; then
    adb push vendor/etc/xtensei-obright.conf /vendor/etc/
fi
echo ""

# Set permissions
echo "Setting permissions..."
adb shell su -c "chmod 0755 /vendor/bin/hw/yamada.obright-V1@3.0-service"
adb shell su -c "chmod 0644 /vendor/etc/init/init.yamada.obright.rc"
adb shell su -c "chmod 0644 /vendor/etc/xtensei-obright.conf 2>/dev/null || true"
echo ""

# Verify
echo "Verifying installation..."
if adb shell su -c "test -f /vendor/bin/hw/yamada.obright-V1@3.0-service && echo OK"; then
    echo "✓ Installation successful!"
    echo ""
    echo "Reboot your device to activate XTENSEI-OBright."
    echo "View logs with: adb logcat | grep XTENSEI-OBright"
else
    echo "✗ Installation failed!"
    exit 1
fi

echo ""
echo "=============================================="
INSTALLER

    chmod +x "${INSTALL_DIR}/install.sh"
    
    # Create uninstall script
    cat > "${INSTALL_DIR}/uninstall.sh" << 'UNINSTALLER'
#!/bin/bash
# XTENSEI-OBright Uninstaller

echo "Uninstalling XTENSEI-OBright..."

adb shell su -c "rm -f /vendor/bin/hw/yamada.obright-V1@3.0-service"
adb shell su -c "rm -f /vendor/etc/init/init.yamada.obright.rc"
adb shell su -c "rm -f /vendor/etc/xtensei-obright.conf"

echo "Files removed. Restore from backup if needed:"
echo "  /data/xtensei-obright-backup/"
echo ""
echo "Reboot your device."
UNINSTALLER

    chmod +x "${INSTALL_DIR}/uninstall.sh"
    
    log_success "Created: ${INSTALL_DIR}/"
    log_info "  Run install.sh to install via ADB"
}

print_summary() {
    log_header "Build Complete"
    
    echo "  Output Directory: ${OUT_DIR}/vendor/"
    echo "  Distribution: ${DIST_DIR}/"
    echo ""
    echo "  Files:"
    echo "    - ${BIN_DIR}/${BIN_NAME}"
    echo "    - ${ETC_DIR}/${RC_NAME}"
    echo "    - ${OUT_DIR}/vendor/etc/${CONFIG_NAME}"
    echo ""
    echo "  Packages:"
    echo "    - ${DIST_DIR}/XTENSEI-OBright-${VERSION}-${CODENAME}-flashable.zip"
    echo "    - ${DIST_DIR}/adb-installer/"
    echo ""
    echo "  Quick Install (ADB):"
    echo "    cd ${DIST_DIR}/adb-installer"
    echo "    ./install.sh"
    echo ""
    echo "  Flash via Recovery:"
    echo "    Flash XTENSEI-OBright-${VERSION}-${CODENAME}-flashable.zip"
    echo ""
    echo "=============================================="
}

# =============================================================================
# Main Build Process
# =============================================================================

main() {
    log_header "XTENSEI-OBright Build Script v${VERSION}"
    echo "  Universal OPlus Brightness Adaptor"
    echo "  for Transsion Devices (TECNO, Infinix, iTel)"
    echo ""

    # Check dependencies and get toolchain path
    TOOLCHAIN=$(check_dependencies)

    # Clean previous build
    cleanup

    # Create output directories
    log_info "Creating output directories..."
    mkdir -p "${BIN_DIR}"
    mkdir -p "${ETC_DIR}"
    mkdir -p "${DIST_DIR}"

    # Build binary
    build_binary "${TOOLCHAIN}"

    # Generate RC file
    generate_rc_file

    # Set permissions
    set_permissions

    # Create distribution packages
    create_flashable_zip
    create_adb_installer

    # Print summary
    print_summary
}

# Run main function
main "$@"
