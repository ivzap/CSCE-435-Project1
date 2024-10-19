import subprocess
from time import sleep

input_sizes = [2**16, 2**18, 2**20, 2**22, 2**24, 2**26]
input_types = ["sorted", "random", "reversed", "perturbed"]
num_procs = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
nodes = [1, 1, 1, 1, 1, 2, 4, 8, 16, 32]

for input_size in input_sizes[0:1]:
    for input_type in input_types[0:1]:
        for i, num_proc in enumerate(num_procs):
            tasks_per_node = num_proc//nodes[i]
            script_template = """#!/bin/bash

##ENVIRONMENT SETTINGS; CHANGE WITH CAUTION
#SBATCH --export=NONE            #Do not propagate environment
#SBATCH --get-user-env=L         #Replicate login environment
#
##NECESSARY JOB SPECIFICATIONS
#SBATCH --job-name=bitonic_sort       #Set the job name to "JobName"
#SBATCH --time=00:04:00           #Set the wall clock limit
#SBATCH --nodes=%s              #Request nodes
#SBATCH --ntasks-per-node=%s    # Request tasks/cores per node
#SBATCH --mem=64G                 #Request GB per node
#SBATCH --output=output.%%j       #Send stdout/err to "output.[jobID]"
#
##OPTIONAL JOB SPECIFICATIONS
##SBATCH --mail-type=ALL              #Send email on all job events
##SBATCH --mail-user=email_address    #Send all emails to email_address
#
##First Executable Line
#

array_size=$1
processes=$2
input_type=$3

module load intel/2020b       # load Intel software stack
module load CMake/3.12.1
module load GCCcore/8.3.0
module load PAPI/6.0.0

CALI_CONFIG="spot(output=p${processes}-a${array_size}-t${input_type}.cali, \
    time.variance,profile.mpi)" \
mpirun -np $processes ./bitonic_sort -p $processes -n $array_size -t $input_type
""" % (nodes[i],tasks_per_node)

            with open('auto.mpi.grace_job', 'w') as file:
                file.write(script_template)

	    sleep(0.5)
            command = ["sbatch", "auto.mpi.grace_job", str(input_size), str(num_proc), str(input_type)]
            subprocess.Popen(command)

