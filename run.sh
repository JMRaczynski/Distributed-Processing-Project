mpicc util.c main.c -o main -Wall -lm
mpirun -np 2 main $1 $2 $3 $4
