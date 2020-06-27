#ifndef SIMUTILSMPI_H
#define SIMUTILSMPI_H

#include <stdbool.h>

#include "spkrcd.h"
//#include "delnet.h"
#include "delnetfixed.h"

#define INPUT_MODE_PERIODIC 1
#define INPUT_MODE_POISSON 2
#define INPUT_MODE_POISSON_EXCLUSIVE 3 		// cuts random noise during input

#define MULTI_INPUT_MODE_SEQUENTIAL 1
#define MULTI_INPUT_MODE_RANDOM 2

#define MAX_NAME_LEN 512

#define getrandom(max1) ((rand()%(int)((max1)))) // random integer between 0 and max-1

/*************************************************************
 *  Structs
 *************************************************************/
typedef struct su_mpi_spike_s {
	unsigned long i;
	double t;
} su_mpi_spike;

typedef struct su_mpi_neuron_s {
	FLOAT_T v;
	FLOAT_T u;
	FLOAT_T a;
	FLOAT_T d;
} su_mpi_neuron;

typedef struct su_mpi_modelparams_s {
	double fs; 				
	double num_neurons; 	
	double p_contact; 		
	double p_exc; 			
	double maxdelay; 		
	double tau_pre; 		
	double tau_post; 		
	double a_pre; 			
	double a_post; 			
	double synmax; 			
	double w_exc; 			
	double w_inh; 			
} su_mpi_modelparams;

typedef struct su_mpi_trialparams_s {
	double dur; 			
	double lambda; 			
	double randspikesize; 	
	bool randinput;
	bool inhibition;
	IDX_T inputmode;
	idx_t multiinputmode;
	double inputweight;
	double recordstart;
	double recordstop;
	double lambdainput;
	double inputrefractorytime;
} su_mpi_trialparams;

typedef struct su_mpi_input_s {
	idx_t len;
	su_mpi_spike *spikes;
} su_mpi_input;

typedef struct su_mpi_model_l_s {
	int commrank;
	int commsize;
	size_t maxnode;
	size_t nodeoffset;
	//IDX_T numsyn;
	su_mpi_modelparams p;
	dnf_delaynet *dn;
	su_mpi_neuron  *neurons;
	FLOAT_T *traces_neu;
	FLOAT_T *traces_syn;
	FLOAT_T *synapses;
} su_mpi_model_l;


/*************************************************************
 *  Function Declarations
 *************************************************************/
/* Helper functions */
void checkfileload(FILE *f, char *name);

/* parameter bookkeeping */
void su_mpi_readmparameters(su_mpi_modelparams *p, char *filename);
void su_mpi_readtparameters(su_mpi_trialparams *p, char *filename);
void su_mpi_printmparameters(su_mpi_modelparams p);
void su_mpi_analyzeconnectivity(unsigned int *g, unsigned int n,
							unsigned int n_exc, FLOAT_T fs);

/* graph generation*/
unsigned int *su_mpi_iblobgraph(su_mpi_modelparams *p);

/* neuron info */
void su_mpi_neuronset(su_mpi_neuron *n, FLOAT_T v, FLOAT_T u, FLOAT_T a, FLOAT_T d);

/* save and load and free models */
void su_mpi_freemodel_l(su_mpi_model_l *m);
void su_mpi_savesynapses(su_mpi_model_l *m, char *name, int commrank, int commsize);
void su_mpi_globalsave(su_mpi_model_l *m_l, char *name, int commrank, int commsize);
su_mpi_model_l *su_mpi_globalload(char *name, int commrank, int commsize);

/* graphs and models */
unsigned long *su_mpi_loadgraph(char *name, su_mpi_model_l *m, int commrank);
su_mpi_model_l *su_mpi_izhiblobstdpmodel(char *mparamfilename, int commrank, int commsize);
su_mpi_model_l *su_mpi_izhimodelfromgraph(char *mparamfilename, char *graphfilename,
									      int commrank, int commsize);

/* run simulations */

/*
void su_mpi_runstdpmodel(su_mpi_model_l *m, su_mpi_trialparams tp,
							su_mpi_spike *input, size_t inputlen,
							spikerecord *sr, char *trialname,
							int commrank, int commsize, bool profiling);
*/
void su_mpi_runstdpmodel(su_mpi_model_l *m, su_mpi_trialparams tp,
							su_mpi_input *inputs, idx_t numinputs, 
							spikerecord *sr, char *trialname,
							int commrank, int commsize, bool profiling);
#endif
