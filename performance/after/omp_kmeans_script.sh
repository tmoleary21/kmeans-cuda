#!/bin/bash
truncate -s 0 return.txt
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
do
    export OMP_NUM_THREADS=$i
    printf "$i\n"
    #for j in 1 2 3 4 5 6 7
    #do
        ./omp_main -a -o -n 5000 -i ./Image_data/colorBig.txt >> return.txt
    #done
    cat return.txt | grep "Computation timing"
    truncate -s 0 return.txt
    printf "\n"
done
