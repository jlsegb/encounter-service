#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
CONFIGURE_ON_MISSING="${CONFIGURE_ON_MISSING:-1}"

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  if [[ "$CONFIGURE_ON_MISSING" != "1" ]]; then
    echo "ERROR: $BUILD_DIR is not configured. Run: cmake -S . -B $BUILD_DIR"
    exit 1
  fi
  echo "== Configure =="
  cmake -S . -B "$BUILD_DIR"
  echo
fi

echo "== Build =="
cmake --build "$BUILD_DIR"
echo

echo "== Test =="
ctest --test-dir "$BUILD_DIR" --output-on-failure
echo

echo "Checks passed."
