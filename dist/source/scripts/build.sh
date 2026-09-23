#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-android"

if [[ -z "${ANDROID_NDK_HOME:-}" ]]; then
  if [[ -n "${ANDROID_NDK:-}" ]]; then
    export ANDROID_NDK_HOME="${ANDROID_NDK}"
  elif [[ -n "${ANDROID_HOME:-}" && -d "${ANDROID_HOME}/ndk" ]]; then
    export ANDROID_NDK_HOME="$(ls -d "${ANDROID_HOME}/ndk"/* 2>/dev/null | sort -V | tail -n 1)"
  fi
fi

if [[ -z "${ANDROID_NDK_HOME:-}" || ! -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then
  echo "Android NDK was not found." >&2
  echo "Set ANDROID_NDK_HOME to your installed NDK and run again." >&2
  exit 2
fi

rm -rf "${BUILD_DIR}"
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release -j"${JOBS:-2}"

python3 "${ROOT}/scripts/package_levipack.py" \
  --library "${BUILD_DIR}/libBlockBreakAnalyzer.so" \
  --icon "${ROOT}/assets/icon.png" \
  --font "${ROOT}/resources/minecraft.ttf" \
  --output "${ROOT}/BlockBreakAnalyzer.levipack"

echo "Built: ${ROOT}/BlockBreakAnalyzer.levipack"
