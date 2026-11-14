# The modules cluster/build.sh and cluster/benchmark.sbatch load: a C++23 compiler, CMake, MPI and Boost. This is the
# Digital Research Alliance of Canada clusters' (Fir, Nibi, Rorqual), where StdEnv/2026 brings GCC and Open MPI over
# UCX; change it for another cluster.

module --force purge
module load StdEnv/2026 cmake boost
module list
