#!/bin/bash
# Runs multiprocessing_benchmark across nodes: 6 nodes x 2 ranks x 2 threads at depth 26. Set your account, build with
# cluster/build.sh, then submit from this repository's root:
#
#   sbatch cluster/benchmark.sbatch
#
#SBATCH --job-name=gempba-cpp
#SBATCH --output=report/%x-%j.out
#SBATCH --error=errors/%x-%j.err
#SBATCH --account=def-someuser
#SBATCH --nodes=6
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=2
#SBATCH --mem-per-cpu=2048M
#SBATCH --time=0-00:15:00

cd "$SLURM_SUBMIT_DIR" || exit
source cluster/modules.sh

# Each rank logs mca_pml_ucx_init when Open MPI runs over UCX.
export OMPI_MCA_pml_ucx_verbose=1

args=(
    -job_name          "$SLURM_JOB_NAME"
    -job_id            "$SLURM_JOB_ID"
    -nodes             "$SLURM_JOB_NUM_NODES"
    -ntasks_per_node   "$SLURM_NTASKS_PER_NODE"
    -ntasks_per_socket -1
    -cpus_per_task     "$SLURM_CPUS_PER_TASK"
    -max_depth         26
    -telemetry_port    9001
    -csv               true
    -I                 Unused
)

export GEMPBA_STOP_TOKEN=${GEMPBA_STOP_TOKEN:-s3cret}  # The token the dashboard's Stop asks for; GEMPBA_STOP_TOKEN in the environment overrides it

echo "Starting run at: $(date)"
srun ./bin/multiprocessing_benchmark "${args[@]}"
echo "Finishing run at: $(date)"
