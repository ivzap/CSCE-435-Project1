#!/bin/bash

module load intel/2020b
module load CMake/3.12.1

cmake \
    -Dadiak_DIR=/scratch/group/csce435-f24/Adiak/adiak/lib/cmake/adiak \
    -Dcaliper_DIR=/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper \
    .

make
