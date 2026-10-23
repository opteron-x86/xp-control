#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --preset native-tests
cmake --build --preset native-tests
ctest --preset native-tests --verbose
