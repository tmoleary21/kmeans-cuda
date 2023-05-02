/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*   File:         omp_new_kmeans.c  (Improved OpenMP version)               */
/*   Description:  Implementation of simple k-means clustering algorithm     */
/*                 This program takes an array of N data objects, each with  */
/*                 M coordinates and performs a k-means clustering given a   */
/*                 user-provided value of the number of clusters (K). The    */
/*                 clustering results are saved in 2 arrays:                 */
/*                 1. a returned array of size [M][K] indicating the center  */
/*                    coordinates of K clusters                              */
/*                 2. membership[N] stores the cluster center ids, each      */
/*                    corresponding to the cluster a data object is assigned */
/*                                                                           */
/*   Author:  Wei-keng Liao                                                  */
/*            ECE Department, Northwestern University                        */
/*            email: wkliao@ece.northwestern.edu                             */
/*                                                                           */
/*   Modified by: Tyson O'Leary, Blake Davis, Chris LaBerge                  */
/*                Computer Science Department, Colorado State University     */
/*                email: tmoleary@colostate.edu                              */
/*                       bdavis43@colostate.edu                              */
/*                       claberge@rams.colostate.edu                         */
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


/*----< kmeans_clustering() >------------------------------------------------*/
/* return an array of cluster centers of size [numClusters][numCoords]       */
int omp_kmeans(int     is_perform_atomic, /* in: */
               float **objects,           /* in: [numObjs][numCoords] */
               int     numCoords,         /* no. coordinates */
               int     numObjs,           /* no. objects */
               int     numClusters,       /* no. clusters */
               float   threshold,         /* % objects change membership */
               int    *membership,        /* out: [numObjs] */
               float **clusters)          /* out: [numCoords][numClusters] */
{
    int      i, j, k, index, loop=0;
    int     *newClusterSize; /* [numClusters]: no. objects assigned in each
                                new cluster */
    float    delta;          /* % of objects change their clusters */
    float  **newClusters;    /* [numCoords][numClusters] */
    double   timing;

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

    if (_debug) timing = omp_get_wtime();
    do {
        delta = 0.0;

        float* distArray;
        #pragma omp parallel\
            private(i,j,index,distArray) \
            firstprivate(numObjs,numClusters,numCoords) \
            shared(objects,clusters,membership,newClusters,newClusterSize)
        {
            distArray = (float*)malloc(numClusters*sizeof(float));
            #pragma omp for schedule(static) reduction(+:delta)
            for (i=0; i<numObjs; i++) {
                float min_dist;

                /* find the cluster id that has min distance to object */
                index    = 0;
                min_dist = INT_MAX;
                int k = 0;

                for (int j=0; j<numClusters; j++){
                    distArray[j] = (objects[i][k]-clusters[k][j]) * (objects[i][k]-clusters[k][j]);
                }

                for (k=1; k<numCoords; k++) {
                    for (int j=0; j<numClusters; j++){
                        distArray[j] += (objects[i][k] - clusters[k][j]) * (objects[i][k] - clusters[k][j]);
                    }
                }

                for (int j=0; j<numClusters; j++){
                    if (distArray[j] < min_dist) { /* find the min and its array index */
                        min_dist = distArray[j];
                        index    = j;
                    }
                }

                /* if membership changes, increase delta by 1 */
                if (membership[i] != index) delta += 1.0;

                /* assign the membership to object i */
                membership[i] = index;

                /* update new cluster centers : sum of objects located within */
                #pragma omp atomic
                newClusterSize[index]++;
                for (j=0; j<numCoords; j++)
                    #pragma omp atomic
                    newClusters[j][index] += objects[i][j];
            }
            free(distArray);
        }

        /* average the sum and replace old cluster centers with newClusters */
        // #pragma omp parallel for private(j)
        for (i=0; i<numClusters; i++) {
            for (j=0; j<numCoords; j++) {
                if (newClusterSize[i] > 1)
                    clusters[j][i] = newClusters[j][i] / newClusterSize[i];
                newClusters[j][i] = 0.0;   /* set back to 0 */
            }
            newClusterSize[i] = 0;   /* set back to 0 */
        }
        delta /= numObjs;
    } while (delta > threshold && loop++ < 500); //500

    if (_debug) {
        timing = omp_get_wtime() - timing;
        printf("nloops = %2d (T = %7.4f)",loop,timing);
    }

    free(newClusters[0]);
    free(newClusters);
    free(newClusterSize);

    return 1;
}

