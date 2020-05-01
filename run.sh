mpicc util.c main.c -o main -Wall -lm
mpirun -np $1 main $2 $3 $4 $5
