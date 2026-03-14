#!/bin/bash
#
# XTENSEI-OBright Build Script
# Builds the OPlus Brightness Adaptor for Transsion Devices
#
# Copyright (C) 2026 XTENSEI
# Licensed under GPL-3.0
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

# Output Configuration
BIN_NAME="yamada.obright-V1@3.0-service"
RC_NAME="init.yamada.obright.rc"
OUT_DIR="out"
BIN_DIR="${OUT_DIR}/vendor/bin/hw"
ETC_DIR="${OUT_DIR}/vendor/etc/init"

# Compiler Flags
CFLAGS="-O3 -Wall -Wextra -Werror -pedantic"
LDFLAGS="-llog -lm"

# =============================================================================
# Helper Functions
# =============================================================================

log_info() {
    echo "[INFO] $1"
}

log_success() {
    echo "[SUCCESS] $1"
}

log_error() {
    echo "[ERROR] $1" >&2
}

log_warn() {
    echo "[WARN] $1" >&2
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
# OPlus Brightness Adaptor for Transsion Devices
#
# Copyright (C) 2026 XTENSEI
# Licensed under GPL-3.0

service xtensei_obright /vendor/bin/hw/${BIN_NAME}
    class hal
    user system
    group system graphics
    capabilities SYS_NICE
    writepid /dev/cpuset/system-background/tasks
    ioprio rt 0

on property:sys.boot_completed=1
    # Set ownership to system user
    chown system system /sys/class/leds/lcd-backlight/brightness

    # Restrict write access to owner only (security)
    chmod 0600 /sys/class/leds/lcd-backlight/brightness

    # Start the brightness service
    start xtensei_obright

on property:vendor.xtensei.obright.restart=1
    start xtensei_obright
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
        log_info "  Size: ${SIZE}"
    else
        log_error "Build failed!"
        exit 1
    fi
}

set_permissions() {
    log_info "Setting file permissions..."
    chmod 0755 "${BIN_DIR}/${BIN_NAME}"
    chmod 0644 "${ETC_DIR}/${RC_NAME}"
    log_success "Permissions set"
}

print_summary() {
    echo ""
    echo "======================================================================"
    echo "  XTENSEI-OBright Build Complete"
    echo "======================================================================"
    echo ""
    echo "  Output Directory: ${OUT_DIR}/vendor/"
    echo ""
    echo "  Files:"
    echo "    - ${BIN_DIR}/${BIN_NAME}"
    echo "    - ${ETC_DIR}/${RC_NAME}"
    echo ""
    echo "  To deploy to device:"
    echo "    adb push ${OUT_DIR}/vendor /vendor"
    echo "    adb shell chmod 0755 /vendor/bin/hw/${BIN_NAME}"
    echo "    adb shell chmod 0644 /vendor/etc/init/${RC_NAME}"
    echo "    adb reboot"
    echo ""
    echo "======================================================================"
}

# =============================================================================
# Main Build Process
# =============================================================================

main() {
    echo "======================================================================"
    echo "  XTENSEI-OBright Build Script"
    echo "  OPlus Brightness Adaptor for Transsion Devices"
    echo "======================================================================"
    echo ""

    # Check dependencies and get toolchain path
    TOOLCHAIN=$(check_dependencies)

    # Clean previous build
    cleanup

    # Create output directories
    log_info "Creating output directories..."
    mkdir -p "${BIN_DIR}"
    mkdir -p "${ETC_DIR}"

    # Build binary
    build_binary "${TOOLCHAIN}"

    # Generate RC file
    generate_rc_file

    # Set permissions
    set_permissions

    # Print summary
    print_summary
}

# Run main function
main "$@"
