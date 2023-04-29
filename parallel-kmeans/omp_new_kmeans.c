/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         kmeans_clustering.c  (OpenMP version)                     */
/*   Description:  Implementation of simple k-means clustering algorithm     */
/*                 This program takes an array of N data objects, each with  */
/*                 M coordinates and performs a k-means clustering given a   */
/*                 user-provided value of the number of clusters (K). The    */
/*                 clustering results are saved in 2 arrays:                 */
/*                 1. a returned array of size [K][N] indicating the center  */
/*                    coordinates of K clusters                              */
/*                 2. membership[N] stores the cluster center ids, each      */
/*                    corresponding to the cluster a data object is assigned */
/*                                                                           */
/*   Author:  Wei-keng Liao                                                  */
/*            ECE Department, Northwestern University                        */
/*            email: wkliao@ece.northwestern.edu                             */
/*                                                                           */
/*   Copyright (C) 2005, Northwestern University                             */
/*   See COPYRIGHT notice in top-level directory.                            */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>

#include <omp.h>
#include <limits.h>
#include "kmeans.h"


/*----< euclid_dist_2() >----------------------------------------------------*/
/* square of Euclid distance between two multi-dimensional points            */
__inline static
float euclid_dist_2(int    numdims,  /* no. dimensions */
                    float *coord1,   /* [numdims] */
                    float *coord2)   /* [numdims] */
{
    int i;
    float ans=0.0;

    for (i=0; i<numdims; i++)
        ans += (coord1[i]-coord2[i]) * (coord1[i]-coord2[i]);

    return(ans);
}

/*----< find_nearest_cluster() >---------------------------------------------*/
__inline static
int find_nearest_cluster(int     numClusters, /* no. clusters */
                         int     numCoords,   /* no. coordinates */
                         float  *object,      /* [numCoords] */
                         float **clusters)    /* [numClusters][numCoords] */
{
    int   index, i;
    float dist, min_dist;

    /* find the cluster id that has min distance to object */
    index    = 0;
    min_dist = euclid_dist_2(numCoords, object, clusters[0]);

    for (i=1; i<numClusters; i++) {
        dist = euclid_dist_2(numCoords, object, clusters[i]);
        /* no need square root */
        if (dist < min_dist) { /* find the min and its array index */
            min_dist = dist;
            index    = i;
        }
    }
    return(index);
}


/*----< kmeans_clustering() >------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
int omp_kmeans(int     is_perform_atomic, /* in: */
               float **objects,           /* in: [numObjs][numCoords] */
               int     numCoords,         /* no. coordinates */
               int     numObjs,           /* no. objects */
               int     numClusters,       /* no. clusters */
               float   threshold,         /* % objects change membership */
               int    *membership,        /* out: [numObjs] */
               float **clusters)          /* out: [numClusters][numCoords] */
{
    int      i, j, k, index, loop=0;
    int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float    delta;          /* % of objects change their clusters */
    float  **newClusters;    /* [numClusters][numCoords] */
    double   timing;

    int      nthreads;             /* no. threads */
    int    **local_newClusterSize; /* [nthreads][numClusters] */
    float ***local_newClusters;    /* [nthreads][numClusters][numCoords] */

    nthreads = omp_get_max_threads();

    /* initialize membership[] */
    for (i=0; i<numObjs; i++) membership[i] = -1;

    /* need to initialize newClusterSize and newClusters[0] to all 0 */
    newClusterSize = (int*) calloc(numClusters, sizeof(int));
    assert(newClusterSize != NULL);

    newClusters    = (float**) malloc(numCoords *            sizeof(float*));
    assert(newClusters != NULL);
    newClusters[0] = (float*)  calloc(numCoords * numClusters, sizeof(float));
    assert(newClusters[0] != NULL);
    for (i=1; i<numCoords; i++)
        newClusters[i] = newClusters[i-1] + numClusters;
// printf("Created numClusters\n");
// fflush(stdout);
    if (!is_perform_atomic) {
        /* each thread calculates new centers using a private space,
           then thread 0 does an array reduction on them. This approach
           should be faster */
        local_newClusterSize    = (int**) malloc(nthreads * sizeof(int*));
        assert(local_newClusterSize != NULL);
        local_newClusterSize[0] = (int*)  calloc(nthreads*numClusters,
                                                 sizeof(int));
        assert(local_newClusterSize[0] != NULL);
        for (i=1; i<nthreads; i++)
            local_newClusterSize[i] = local_newClusterSize[i-1]+numClusters;

        /* local_newClusters is a 3D array */
        local_newClusters    =(float***)malloc(nthreads * sizeof(float**));
        assert(local_newClusters != NULL);
        local_newClusters[0] =(float**) malloc(nthreads * numClusters *
                                               sizeof(float*));
        assert(local_newClusters[0] != NULL);
        for (i=1; i<nthreads; i++)
            local_newClusters[i] = local_newClusters[i-1] + numClusters;
        for (i=0; i<nthreads; i++) {
            for (j=0; j<numClusters; j++) {
                local_newClusters[i][j] = (float*)calloc(numCoords,
                                                         sizeof(float));
                assert(local_newClusters[i][j] != NULL);
            }
        }
    }

    if (_debug) timing = omp_get_wtime();
    do {
        delta = 0.0;

        if (is_perform_atomic) {
            // printf("Entered if(is_perform_atomic)\n");
            // fflush(stdout);
            float* distArray;
            #pragma omp parallel\
                private(i,j,index,distArray) \
                firstprivate(numObjs,numClusters,numCoords) \
                shared(objects,clusters,membership,newClusters,newClusterSize)
            {
                distArray = (float*)malloc(numClusters*sizeof(float));
                #pragma omp for schedule(static) reduction(+:delta)
                for (i=0; i<numObjs; i++) {
                    // printf("i=%d\n", i);
                    // fflush(stdout);
                    float min_dist;

                    /* find the cluster id that has min distance to object */
                    index    = 0;
                    min_dist = INT_MAX;
                    int k = 0;

                    int current_object_coord = objects[k][i];
                    for (int j=0; j<numClusters; j++){
                        distArray[j] = (current_object_coord-clusters[k][j]) * (current_object_coord-clusters[k][j]);

                        if (k == numCoords-1 && distArray[j] < min_dist) { /* find the min and its array index */
                            min_dist = distArray[j];
                            index    = j;
                        }
                    }

                    // printf("Passed k=0 loop\n");
                    // fflush(stdout);

                    for (k=1; k<numCoords; k++) {
                        current_object_coord = objects[k][i];
                        for (int j=0; j<numClusters; j++){
                            distArray[j] += (current_object_coord - clusters[k][j]) * (current_object_coord - clusters[k][j]);
                            
                            if (k == numCoords-1 && distArray[j] < min_dist) { /* find the min and its array index */
                                min_dist = distArray[j];
                                index    = j;
                            }
                        }
                    }

                    // printf("Passed rest of k/j loop\n");
                    // fflush(stdout);

                    /* if membership changes, increase delta by 1 */
                    if (membership[i] != index) delta += 1.0;

                    // printf("a\n");
                    // fflush(stdout);
                    /* assign the membership to object i */
                    membership[i] = index;
                    printf("Membership for object %d is %d\n",i,index);
                    // fflush(stdout);

                    /* update new cluster centers : sum of objects located within */
                    // printf("a\n");
                    // fflush(stdout);
                    #pragma omp atomic
                    newClusterSize[index]++;
                    // printf("a\n");
                    // fflush(stdout);
                    for (j=0; j<numCoords; j++)
                        #pragma omp atomic
                        newClusters[j][index] += objects[j][i];
                    // printf("Passed atomic loop\n");
                    // fflush(stdout);
                }
                free(distArray);
            }
        }
        else {
            #pragma omp parallel \
                    shared(objects,clusters,membership,local_newClusters,local_newClusterSize)
            {
                int tid = omp_get_thread_num();
                #pragma omp for \
                            private(i,j,index) \
                            firstprivate(numObjs,numClusters,numCoords) \
                            schedule(static) \
                            reduction(+:delta)
                for (i=0; i<numObjs; i++) {
                    /* find the array index of nestest cluster center */
                    index = find_nearest_cluster(numClusters, numCoords,
                                                 objects[i], clusters);

                    /* if membership changes, increase delta by 1 */
                    if (membership[i] != index) delta += 1.0;

                    /* assign the membership to object i */
                    membership[i] = index;

                    /* update new cluster centers : sum of all objects located
                       within (average will be performed later) */
                    local_newClusterSize[tid][index]++;
                    for (j=0; j<numCoords; j++)
                        local_newClusters[tid][index][j] += objects[i][j];
                }
            } /* end of #pragma omp parallel */

            /* let the main thread perform the array reduction */
            for (i=0; i<numClusters; i++) {
                for (j=0; j<nthreads; j++) {
                    newClusterSize[i] += local_newClusterSize[j][i];
                    local_newClusterSize[j][i] = 0.0;
                    for (k=0; k<numCoords; k++) {
                        newClusters[i][k] += local_newClusters[j][i][k];
                        local_newClusters[j][i][k] = 0.0;
                    }
                }
            }
        }

        /* average the sum and replace old cluster centers with newClusters */
        #pragma omp parallel for private(j)
        for (i=0; i<numClusters; i++) {
            for (j=0; j<numCoords; j++) {
                if (newClusterSize[i] > 1)
                    clusters[j][i] = newClusters[j][i] / newClusterSize[i];
                newClusters[j][i] = 0.0;   /* set back to 0 */
            }
            newClusterSize[i] = 0;   /* set back to 0 */
        }
        // printf("Average the sum loop passed\n");
        // fflush(stdout);
        delta /= numObjs;
    } while (delta > threshold && loop++ < 500);

    if (_debug) {
        timing = omp_get_wtime() - timing;
        printf("nloops = %2d (T = %7.4f)",loop,timing);
    }

    if (!is_perform_atomic) {
        free(local_newClusterSize[0]);
        free(local_newClusterSize);

        for (i=0; i<nthreads; i++)
            for (j=0; j<numClusters; j++)
                free(local_newClusters[i][j]);
        free(local_newClusters[0]);
        free(local_newClusters);
    }
    free(newClusters[0]);
    free(newClusters);
    free(newClusterSize);

    return 1;
}

