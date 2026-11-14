#!/bin/bash
# Build multiprocessing_benchmark against this cluster's MPI. Expects gempba checked out next to this repository. Run
# it on a login node:
#
#   bash cluster/build.sh
#
# fmt, spdlog and hwloc come from gempba's build_deps.sh, since a cluster's modules can be older than gempba needs.

set -euo pipefail

examples=$(cd "$(dirname "$0")/.." && pwd)
gempba=$(cd "$examples/../gempba" && pwd)
deps="$gempba/.build-jni-deps"

# shellcheck source=modules.sh
source "$examples/cluster/modules.sh"
echo "gempba: $gempba on $(git -C "$gempba" branch --show-current)"

bash "$gempba/bindings/jni/build_deps.sh" "$deps"
rm -f "$examples/.build-cluster/CMakeCache.txt"
PKG_CONFIG_PATH="$deps/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" cmake -S "$examples" -B "$examples/.build-cluster" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$deps" -DGEMPBA_EXAMPLES_MPI=ON -DGEMPBA_HWLOC=ON
cmake --build "$examples/.build-cluster" --target multiprocessing_benchmark -j4

mkdir -p "$examples/report" "$examples/errors" "$examples/results"
echo "Built $examples/bin/multiprocessing_benchmark; submit with: sbatch cluster/benchmark.sbatch"
