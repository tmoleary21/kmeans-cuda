# kmeans-OpenMP
Final project from CS475 Parallel Programming. Improving an existing k-means clustering implementation with an OpenMP parallelization.

The `parallel-kmeans` directory contains our improved code as well as the base code we are building off of. Source: [http://users.eecs.northwestern.edu/~wkliao/Kmeans/index.html](http://users.eecs.northwestern.edu/~wkliao/Kmeans/index.html)

Our modified files are `omp_new_kmeans.c` and `omp_new_main.c`.

To run our program, you MUST use the Intel ICC compiler.<br>
source /s/parsons/l/sys/intel/mkl/bin/mklvars.sh intel64<br>
export PATH="/s/parsons/l/sys/intel_2020/compilers_and_libraries_2020.4.304/linux/bin/intel64/:$PATH"

Then run using the following command inside of the parallel-kmeans directory:<br>
./omp_new_main -a -o -n 5000 -i ./Image_data/colorBig.txt

For more information on how to run our program, please see the README file inside of the parallel-kmeans directory.

A PowerPoint Presentation and Report of our work can be found in the documentation folder.
