#!/usr/bin/env bash
# Configures, builds and runs the host-side unit tests.
#
# These build for the HOST, not for AVR, so they run anywhere with a C compiler
# and need no hardware. Sanitizers (ASan + UBSan) are always on - several tests
# rely on them to catch out-of-bounds access rather than on an assertion.
#
# Exits non-zero if any test fails, so it is safe to use as a CI gate or a
# pre-push check.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/tests"

cmake -S "${REPO_ROOT}/tests" -B "${BUILD_DIR}" -G Ninja "$@"
cmake --build "${BUILD_DIR}" --target test_all
