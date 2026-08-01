#!/bin/bash
set -euo pipefail

rm -rf build
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
  "$@"

cmake --build build
