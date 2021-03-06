#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stddef.h>
#include <mpi.h>

#include "simutils.h"
#include "spkrcd.h"
#include "inputs.h"

#define PROFILING 1
#define DN_MAIN_DEBUG 0

#define DN_TRIALTYPE_NEW 0
#define DN_TRIALTYPE_RESUME 1

#define MAX_NAME_LEN 512


/*************************************************************
 *  Main
 *************************************************************/
int main(int argc, char *argv[])
{
    su_mpi_model_l *m;
    su_mpi_trialparams tp;
    int commsize, commrank;

    /* Init MPI */
    MPI_Init(&argc, &argv); 
    MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &commrank);

    /* Set up MPI Datatype for spikes */
    MPI_Datatype mpi_spike_type = sr_commitmpispiketype();

    /* Check number of inputs (needs three) */
    if (argc != 4) {
        printf("Input: Trial type (0 or 1), input name, output name.\n");
        exit(-1);
    }

    char *in_name = argv[2];
    char *out_name = argv[3];
    int trialtype = atoi(argv[1]);

    /* model create or load */
    if (trialtype == DN_TRIALTYPE_NEW) {
        m = su_mpi_izhimodelfromgraph(in_name, commrank, commsize);
        if (DN_MAIN_DEBUG) printf("Made model on process %d\n", commrank);
    } else if (trialtype == DN_TRIALTYPE_RESUME) {
        m = su_mpi_globalload(in_name, commrank, commsize);
        if (DN_MAIN_DEBUG) printf("Loaded model on process %d\n", commrank);
    } else {
        printf("First argument must be 0 or 1. Exiting.\n");
        exit(-1);
    }

    /* process trial parameters */
    if (commrank == 0) {
        su_mpi_readtparameters(&tp, in_name);
        MPI_Bcast( &tp, sizeof(su_mpi_trialparams), MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        MPI_Bcast( &tp, sizeof(su_mpi_trialparams), MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    if (DN_MAIN_DEBUG) printf("Loaded trial parameters on process %d\n", commrank);

    /* set up spike recorder on each rank */
    char srname[MAX_NAME_LEN];
    char rankstr[MAX_NAME_LEN];
    sprintf(rankstr, "_%d_%d", commsize, commrank);
    strcpy(srname, out_name);
    strcat(srname, rankstr);
    strcat(srname, "_spikes.txt");
    spikerecord *sr = sr_init(srname, SPIKE_BLOCK_SIZE);

    /* load input sequence on each rank */
    su_mpi_input *forced_inputs = 0;
    long int numinputs = loadinputs(in_name, &forced_inputs, mpi_spike_type, m, commrank);


    /* run simulation */
    if (DN_MAIN_DEBUG) printf("Running simulation on rank %d\n", commrank);
    if (tp.stdp) {
        su_mpi_runstdpmodel(m, tp, forced_inputs, numinputs,
                            sr, out_name, commrank, commsize, PROFILING);
    }
    else {
        su_mpi_runmodel(m, tp, forced_inputs, numinputs,
                        sr, out_name, commrank, commsize, PROFILING);
    }


    /* save resulting model state */
    if (DN_MAIN_DEBUG) printf("Saving synapses on rank %d\n", commrank);
    su_mpi_savesynapses(m, out_name, commrank, commsize);
    if (DN_MAIN_DEBUG) printf("Saving model state on rank %d\n", commrank);
    su_mpi_globalsave(m, out_name, commrank, commsize);

    /* Clean up */
    char srfinalname[256];
    strcpy(srfinalname, out_name);
    strcat(srfinalname, "_spikes.txt");

    if (DN_MAIN_DEBUG) printf("Saving spikes on rank %d\n", commrank);
    sr_collateandclose(sr, srfinalname, commrank, commsize, mpi_spike_type);

    su_mpi_freemodel_l(m);
    freeinputs(&forced_inputs, numinputs);
    MPI_Finalize();

    return 0;
}
