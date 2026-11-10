#!/bin/bash

# Build the example tree on Windows / MSYS2. CMake handles gempba resolution:
# an installed gempba on CMAKE_PREFIX_PATH is preferred; otherwise a sibling
# checkout at ../gempba is folded in via add_subdirectory.
#
# Usage:
#   bash scripts/build_windows.sh [BUILD_TYPE] [MPI] [HWLOC]
#
# Examples:
#   bash scripts/build_windows.sh                        # Release, MPI examples ON, hwloc ON
#   bash scripts/build_windows.sh Debug                  # Debug, MPI examples ON, hwloc ON
#   bash scripts/build_windows.sh Release OFF            # Release, multithreading examples only
#   bash scripts/build_windows.sh Debug ON OFF           # Debug, MPI examples ON, hwloc OFF

set -euo pipefail

WORKSPACE=$(cd "$(dirname "$0")/.." && pwd)
BUILD_TYPE="${1:-Release}"
MPI="${2:-ON}"
HWLOC="${3:-ON}"

cmake -G "MSYS Makefiles" -S "$WORKSPACE" -B "$WORKSPACE/.build" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DGEMPBA_EXAMPLES_MPI="$MPI" \
    -DGEMPBA_HWLOC="$HWLOC"
cmake --build "$WORKSPACE/.build" --parallel "$(nproc)"
