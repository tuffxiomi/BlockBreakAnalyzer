#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-android"

# Find Android NDK
if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
    if [[ -n "${ANDROID_NDK:-}" ]]; then
        export ANDROID_NDK_HOME="${ANDROID_NDK}"
    elif [[ -n "${ANDROID_HOME:-}" && -d "${ANDROID_HOME}/ndk" ]]; then
        export ANDROID_NDK_HOME="$(
            ls -d "${ANDROID_HOME}/ndk"/* 2>/dev/null |
            sort -V |
            tail -n 1
        )"
    fi
fi

if [[ -z "${ANDROID_NDK_HOME:-}" ||
      ! -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then

    echo "ERROR: Android NDK was not found."
    echo
    echo "Set:"
    echo "  export ANDROID_NDK_HOME=/path/to/android-ndk"
    echo
    exit 2
fi

echo "[1/4] Cleaning previous Android build..."
rm -rf "${BUILD_DIR}"

echo "[2/4] Configuring ARM64 Android build..."

cmake \
    -S "${ROOT}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release

echo "[3/4] Compiling BlockBreakAnalyzer..."

cmake \
    --build "${BUILD_DIR}" \
    --config Release \
    -j"${JOBS:-2}"

LIB="${BUILD_DIR}/libBlockBreakAnalyzer.so"

if [[ ! -f "${LIB}" ]]; then
    echo "ERROR: Build completed but .so was not produced."
    exit 3
fi

echo "[4/4] Creating LeviPack..."

python3 \
    "${ROOT}/scripts/package_levipack.py" \
    --library "${LIB}" \
    --icon "${ROOT}/assets/icon.png" \
    --font "${ROOT}/resources/minecraft.ttf" \
    --output "${ROOT}/BlockBreakAnalyzer.levipack"

if [[ ! -f "${ROOT}/BlockBreakAnalyzer.levipack" ]]; then
    echo "ERROR: LeviPack packaging failed."
    exit 4
fi

echo
echo "======================================"
echo " BlockBreakAnalyzer build successful"
echo "======================================"
echo
echo "Library:"
echo "  ${LIB}"
echo
echo "LeviPack:"
echo "  ${ROOT}/BlockBreakAnalyzer.levipack"
