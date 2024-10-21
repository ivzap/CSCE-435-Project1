#!/bin/bash


#arrays for input_size, input_type, and num_procs
input_sizes=(65536 262144 1048576 4194304 16777216 67108864)
input_types=("Sorted" "Random" "ReverseSorted" "1_perc_perturbed")
num_procs=1024

# Loop through all combinations
for size in "${input_sizes[@]}"; do
  for type in "${input_types[@]}"; do
    echo "Submitting job with input_size=$size, input_type=$type, num_procs=$num_procs"
    sbatch merge_mpi.grace_job $size $type $num_procs
  done
done

