#!/usr/bin/env bash
# ==============================================================================
# Modern 3D Game Engine (Standard 2026) - Linux Build & Test Automation Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=== Building Modern 3D Game Engine on Linux (Clang 18 + Vulkan 1.3) ==="

BUILD_DIR="${ROOT_DIR}/build_linux"
BUILD_TYPE="${1:-Release}"

mkdir -p "${BUILD_DIR}"

cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DBUILD_EDITOR=OFF \
    -DBUILD_TESTS=ON \
    -DENABLE_LTO=ON

ninja -C "${BUILD_DIR}" -j"$(nproc)"

echo "=== Running CTest Unit Tests ==="
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "=== Linux Build & Verification Completed Successfully ==="
