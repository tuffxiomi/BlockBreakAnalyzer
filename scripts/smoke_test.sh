#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build-host"
rm -rf "${BUILD_DIR}"
cmake -S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"${JOBS:-2}"
test -f "${BUILD_DIR}/libBlockBreakAnalyzer.so"
python3 "${ROOT}/tests/verify_package_layout.py" "${ROOT}"
echo "Host smoke test passed."
