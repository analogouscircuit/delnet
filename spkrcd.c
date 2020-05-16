#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "spkrcd.h"
#ifdef __amd64__
#include "/usr/lib/x86_64-linux-gnu/openmpi/include/mpi.h"
#else
#include <mpi.h>
#endif


#if SIZE_MAX == UCHAR_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif

spikerecord *sr_init(char *filename, size_t spikes_in_block)
{
	spikerecord *sr;

	sr = malloc(sizeof(spikerecord));
	sr->blocksize = spikes_in_block;
	sr->numspikes = 0;
	sr->blockcount = 0;
	sr->filename = filename;
	sr->writemode = "w";
	sr->spikes = malloc(sizeof(spike)*spikes_in_block);

	return sr;
}

void sr_save_spike(spikerecord *sr, int neuron, SR_FLOAT_T time)
{
	if (sr->blockcount < sr->blocksize) {
		sr->spikes[sr->blockcount].neuron = neuron;
		sr->spikes[sr->blockcount].time = time;
		sr->blockcount += 1;
		sr->numspikes += 1;
	} 
	else {
		FILE *spike_file;
		spike_file = fopen(sr->filename, sr->writemode);
		for (int i=0; i < sr->blocksize; i++) {
			fprintf(spike_file, "%f  %d\n", 
					sr->spikes[i].time,
					sr->spikes[i].neuron);	
		}
		fclose(spike_file);

		sr->writemode = "a";
		sr->spikes[0].neuron = neuron;
		sr->spikes[0].time = time;
		sr->blockcount = 1;
		sr->numspikes += 1;
	}
}

int *len_to_offsets(int *lens, int n)
{
	int *offsets = calloc(n, sizeof(int));

	for (int i=1; i<n; i++) {
		for (int j=1; j<=i; j++) {
			offsets[i] += lens[j-1];
		}
	}
	return offsets;
}


static spike *s1 = 0;
static spike *s2 = 0;
static float diff = 0.0;

int spkcomp(const void *spike1, const void * spike2) {
	s1 = (spike *)spike1;
	s2 = (spike *)spike2;
	diff = s1->time - s2->time;

	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;
	else 
		return s1->neuron - s2->neuron;
}

/*
 * Read spikes written by each process into memory, sort by time,
 * write into a unified file, delete individual files. 
 *
 * Very naive, essentially sequential approach, refine later.
 */
void sr_collateandclose(spikerecord *sr, char *finalfilename, int commrank, int commsize)
{
	/* Set up MPI Datatype for spikes */
	const int 		nitems = 2;
	int 			blocklengths[2] = {1, 1};
	MPI_Datatype	types[2] = {MPI_INT, MPI_FLOAT}; 	// <- think float is double -- check
	MPI_Datatype	mpi_spike_type;
	MPI_Aint  		offsets[2];

	offsets[0] = offsetof(spike, neuron);
	offsets[1] = offsetof(spike, time);
	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_spike_type);
	if (MPI_SUCCESS != MPI_Type_commit(&mpi_spike_type)) {
		printf("Failed to commit custom MPI type!\n");
		exit(-1);
	}

	/* Each rank finishing writing its local file */		
	FILE *spike_file;
	spike_file = fopen(sr->filename, sr->writemode);
	for (int i=0; i < sr-> numspikes % sr->blocksize; i++) {
		fprintf(spike_file, "%f  %d\n", 
				sr->spikes[i].time,
				sr->spikes[i].neuron);	
	}
	fclose(spike_file);

	/* Gather the numbers of neurons to read/write from each rank */
	int *rankspikecount = 0;
	int *spikeoffsets = 0;
	int numspikestotal = 0;
	spike *allspikes = 0;
	spike *localspikes = malloc(sizeof(spike)*sr->numspikes);

	if (commrank == 0)
		rankspikecount = malloc(sizeof(int)*commsize);

	MPI_Gather(&sr->numspikes, 1, MPI_INT,
			   rankspikecount, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (commrank == 0) {
		spikeoffsets = len_to_offsets(rankspikecount, commsize);
		for (int i=0; i<commsize; i++)
			numspikestotal += rankspikecount[i];
		allspikes = malloc(sizeof(spike)*numspikestotal);
	}
	

	/* Read spikes back into memory and delete process-local file*/
	size_t i = 0;
	fopen(sr->filename, "r");
	while(fscanf(spike_file, "%f  %d", &localspikes[i].time, &localspikes[i].neuron) != EOF)
		i++;
	if (i != sr->numspikes) {
		printf("Read %lu spikes, but should have been %lu spikes. Exiting.\n", i, sr->numspikes);
		exit(-1);
	}
	fclose(spike_file);
	remove(sr->filename);

	/* Gather all spikes on a single rank, sort and write (parallelize later) */
	MPI_Gatherv(localspikes, sr->numspikes, mpi_spike_type, 
				allspikes, rankspikecount, spikeoffsets,
				mpi_spike_type, 0, MPI_COMM_WORLD);

	if (commrank == 0) {
		qsort(allspikes, numspikestotal, sizeof(spike), spkcomp);
		spike_file = fopen(finalfilename, "w");
		for (int j=0; j<numspikestotal; j++) 
			fprintf(spike_file, "%lf  %d\n", allspikes[j].time, allspikes[j].neuron);
		fclose(spike_file);
	}

	/* Clean up */
	free(sr->spikes);
	free(sr);
	free(localspikes);
	if (commrank == 0) {
		free(rankspikecount);
		free(spikeoffsets);
		free(allspikes);
	}
}


/*
 * TO BE DEPRECATED.  Allows all ranks to write their own spike file.
 */
void sr_close(spikerecord *sr)
{
	/* write any as-of-yet unsaved spikes */	
	FILE *spike_file;
	spike_file = fopen(sr->filename, sr->writemode);
	for (int i=0; i < sr-> numspikes % sr->blocksize; i++) {
		fprintf(spike_file, "%f  %d\n", 
				sr->spikes[i].time,
				sr->spikes[i].neuron);	
	}
	fclose(spike_file);

	/* free memory */
	free(sr->spikes);
	free(sr);
}
