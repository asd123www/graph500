#!/bin/bash
#SBATCH -o job_%j_%N.out
#SBATCH -p cpu
#SBATCH -J hw2
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH -t 5:00
module add mpich
mpirun -np 2 ./graph500_custom_bfs 23 16
# mpirun -np 8 ./graph500_reference_bfs 23 16
#  the last two arguments:
#  SCALE = log_2(# vertices) [integer, required]
#  edgefactor = (# edges) / (# vertices) = .5 * (average vertex degree) [integer, defaults to 16]
