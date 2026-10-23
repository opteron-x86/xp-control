#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "${OS:-}" != Windows_NT ]]; then
  echo 'Use the GitHub Windows build, or configure a Windows cross-toolchain with matching dependencies.' >&2
  exit 1
fi
cmake --preset windows-release
cmake --build --preset windows-release
ctest --test-dir build/windows-release --verbose
python tools/package.py --build build/windows-release --output dist
