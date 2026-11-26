#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN="${BUILD_DIR}/liquid-glass-ui"

if [[ ! -d "${BUILD_DIR}" ]]; then
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
fi

cmake --build "${BUILD_DIR}"

cd "${SCRIPT_DIR}"
exec "${BIN}" "$@"
