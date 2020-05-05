#ifndef SIMUTILSMPI_H
#define SIMUTILSMPI_H

#include <stdbool.h>

#include "spkrcd.h"
#include "delnet.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/*************************************************************
 *  Structs
 *************************************************************/
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
	double fs; 				
	double dur; 			
	double lambda; 			
	double randspikesize; 	
	bool randinput;
	bool inhibition;
	IDX_T numinputs;
	IDX_T *inputidcs;
} su_mpi_trialparams;

typedef struct su_mpi_model_l_s {
	IDX_T numinputneurons;
	int commrank;
	int commsize;
	size_t maxnode;
	size_t nodeoffset;
	IDX_T numsyn;
	//IDX_T numsyn_exc;
	su_mpi_modelparams p;
	dn_mpi_delaynet *dn;
	su_mpi_neuron  *neurons;
	FLOAT_T *traces_neu;
	FLOAT_T *traces_syn;
	FLOAT_T *synapses;
	IDX_T *neuronlookup;
} su_mpi_model_l;

/*************************************************************
 *  Function Declarations
 *************************************************************/

/* parameter bookkeeping */
EXTERNC void su_mpi_setdefaultmparams(su_mpi_modelparams *p);
EXTERNC void su_mpi_readmparameters(su_mpi_modelparams *p, char *filename);
EXTERNC void su_mpi_readtparameters(su_mpi_trialparams *p, char *filename);
EXTERNC void su_mpi_printmparameters(su_mpi_modelparams p);
EXTERNC void su_mpi_analyzeconnectivity(unsigned int *g, unsigned int n,
							unsigned int n_exc, FLOAT_T fs);

/* graph generation*/
EXTERNC unsigned int *su_mpi_iblobgraph(su_mpi_modelparams *p);

/* neuron info */
EXTERNC void su_mpi_neuronset(su_mpi_neuron *n, FLOAT_T v, FLOAT_T u, FLOAT_T a, FLOAT_T d);

/* save and load and free models */
EXTERNC void su_mpi_savemodel_l(su_mpi_model_l *m, char *filename);
EXTERNC su_mpi_model_l *su_mpi_loadmodel_l(char *filename);
EXTERNC void su_mpi_freemodel_l(su_mpi_model_l *m);

/* generate model */
EXTERNC su_mpi_model_l *su_mpi_izhiblobstdpmodel(char *mparamfilename, int commrank, int commsize);

/* run simulations */
EXTERNC void su_mpi_runstdpmodel(su_mpi_model_l *m, su_mpi_trialparams tp, FLOAT_T *input, size_t inputlen,
					spikerecord *sr, int commrank, int commsize, bool profiling);


#endif
