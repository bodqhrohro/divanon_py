/*	Cascade Correlation Learning Algorithm

	v1.1
	Matt White  (mwhite+@cmu.edu)
        5/31/93
*/

#ifndef CASCOR
#define CASCOR

/*	Include Files	*/

#include "tools.h"
#include "parse.h"


/*	Data Enumeration Definitions	*/

typedef enum  {		/*  This enumeration describes the four modes the  */ 
  TRAINING,		/* network can enter into during training.	   */
  TIMEOUT,		/* TRAINING means that the network is still        */
  STAGNANT,		/* training.  TIMEOUT means that the network       */
  WIN			/* reached the maximum number of epochs for the    */
}  status_t;		/* last training cycle.  STAGNANT meand that,	   */
			/* during the last training cycle,  a point was    */
/* reached where performance ceased to improve by an appreciable amount.   */
/* WIN simply means that network performance reached an acceptable level   */
/* during the last training cycle.					   */


typedef enum  {		/*  error_t describes the two methods for 	*/
  BITS,			/* calculating error.  Either the program can   */
  INDEX			/* count the number of responses that are	*/
}  error_t;		/* wrong, or it can sum overall error among all */
			/* the outputs.					*/


typedef enum  {		/*  An enumeration of the various unit types.	*/
  VARIED,		/* Each unit type has a specific activation	*/
  UNINITIALIZED,	/* function associated with it.  VARIED		*/
  SIGMOID,		/* indicates a mix of the other unit types	*/
  ASIGMOID,
  GAUSSIAN,
  VARSIGMOID,
  LINEAR
}  unit_type;


/*	Data Structure Definitions	*/

/*	NET_DATA -  Holds weights and values for the network as a whole
*/

typedef struct  {	
  unit_type  *unitTypes;  /*  Unit types for each unit in the network       */
  float      *values,	  /*  Activation value of each unit in the network  */
            **weights;	  /*  Value of weights connecting the units	  */
}  net_data;


/*	OUTPUT_DATA -  Holds values, weights and training information about the
	outputs of the network.
*/

typedef struct  {
  float      shrinkFactor,	/*  Shrink factor to be used by quickprop    */
             scaledEpsilon,	/*  Output epsilon, scaled by the number of  */
				/* training points being used.		     */
             *values,		/*  Activation value of each output	     */
             **weights,		/*  Weights connecting each output to inputs */
				/* and hidden units			     */
             **deltas,		/*  Change made in this particular weight    */
				/* last epoch.  Used in quickprop.	     */
             **slopes,		/*  Partial derivitive of error with respect */
				/* to the weight.			     */
             **pSlopes;		/*  Previous partial derivitive		     */
  unit_type  *types;		/*  Unit type of each output		     */
}  output_data;


/*	CAND_DATA -  This structure contains information to the training and
	selecting of new units to be added to the network.
*/

typedef struct  {
  int        best;		/*  Number of the current best unit	    */
  float      shrinkFactor,	/*  Shrink factor for candidates	    */
             bestScore,		/*  Correlation value for the current best  */
				/* candidate				    */
             *values,		/*  Activation value of each candidate	    */
             *sumVals,		/*  Summed activation over all training	    */
				/* examples.				    */
             **weights,		/*  Weight values between candidates and    */
				/* inputs and hidden units		    */
             **corr,		/*  Correlation between candidates and each */
				/* output in the network		    */
             **pCorr,		/*  Previous correlation values		    */
             **deltas,		/*  Change made in this weight last epoch   */
             **slopes,		/*  Slope of the correlation to weight	    */
				/* curve.  Used in quickprop.		    */
             **pSlopes;		/*  Previous values of the slope	    */
  unit_type  *types;		/*  Type of each individual candidate	    */
}  cand_data;


/*	ERROR_DATA -  This structure contains information used to determine how
	well the network is doing.  Anything to do with the network's error.
*/

typedef struct  {
  int      bits;		/*  Number of bits incorrect		     */
  float    stdDev,		/*  Standard deviation of all training	     */
				/* points being used			     */
           index,		/*  Error index.  Used to determine whether  */
				/* a network is done training.		     */
           indexThresh,		/*  How good the error index has to get      */
				/* before training is finished		     */
           trueErr,		/*  Sum squared raw error		     */
           sumSqErr,		/*  Sum squared error prime		     */
           scoreThresh,		/*  How close an output has to be to its     */
				/* correct value to be considered correct.   */
				/* Used in BITS error measure.		     */
           *sumErr,		/*  The sum of the errors for all training   */
				/* points, by output			     */
           *errors;		/*  Error at each output		     */
  error_t  measure;		/*  Which method to use to calculate error   */
}  error_data;


/*	NODE_PARMS -  This structure holds parameter values for both the
	candidate as well as outputs.
*/

typedef struct  {
  int    epochs,		/*  Number of epochs to train before timeout */
         patience;		/*  How long to continue training without    */
				/* noticable improvement before calling the  */
				/* cycle stagnant			     */
  float  sigMax,		/*  Maximum value for VARSIGMOID units to    */
				/* stretch to 			             */
         sigMin,		/*  Minimum value for VARSIGMOID units to    */
				/* stretch to				     */
         epsilon,		/*  Epsilon, or learning rate to use	     */
         decay,			/*  Slope decay per epoch.  Controls         */
				/* floating point overflow.		     */
	 mu,			/*  Maximum growth factor.  Quickprop	     */
         changeThresh;		/*  Determines what is considered noticable  */
				/* change for stagnation purposes	     */
}  node_parms;


/*	PARM_DATA -  This structure contains user set parameters used by the
	program, except those that pertain to error.  Everything that should
	be user settable, with the exception of error parameters, is located
	withing this structure.
*/

typedef struct  {
  int         Ntrials,		/*  Number of trials to run                  */
              maxNewUnits,	/*  Maximum number of new units to add to    */
			        /* the network				     */
              winRadius,	/*  Radius of window to use on sequence      */
				/* data sets				     */
              valPatience;	/*  Number of cycles to continue without     */
				/* improvement on validation set performance */
				/* before stopping trial		     */
  float       weightRange,	/*  Range of initial random weights	     */
              weightMult,	/*  Multiplier for generating weights 	     */
				/* between a new unit and the outputs	     */
              sigPrimeOffset,	/*  Used to eliminate offset in sigmoid      */
				/* activation functions			     */
              bias;		/*  Activation level for the bias unit	     */
  char        *dataFile,	/*  Name of the data file being used	     */
              *weightFile;	/*  Name of the weight file being used       */
  boolean     parseInBinary,	/*  Parse enumerated network inputs as	     */
				/* binary values as opposed to unary values  */
              parseOutBinary,	/*  Parse enumerated network outputs as	     */
				/* binary values instead of unary values     */
              validate,		/*  Perform a validation epoch every cycle   */
              test,		/*  Perform a test epoch every trial	     */
              useCache,		/*  Cache activation values and errors	     */
              saveWeights;	/*  Save the weights at the end of each      */
				/* trial				     */
  unit_type   candNewType;	/*  Type of candidates to use in cand pool   */
  node_parms  out,		/*  Output unit parameters		     */
              cand;		/*  Candidate unit parameters		     */
}  parm_data;


/*	ALT_DATA_T -  Holds information about alternate data sets in the data
	file.  Namely this means the validation and test sets.  This structure
	is essential to being able to validate and test the network.
*/

typedef struct  {
  int       Npts,		/*  Number of points, total, in this set     */
            Nsegs,		/*  Number of segment markers in this set    */
            NoutVals,		/*  Number of total output values in the set */
            bestPass;		/*  Validation only, which cycle was best?   */
  float     stdDev,		/*  Standard deviation of the set	     */
            bestScore,		/*  True error value at the end of the best  */
				/* cycle				     */
            **bestOutConn;	/*  Value of each weight connecting inputs   */
				/* and hidden units to the outputs during    */
				/* the 'best' cycle			     */
  data_set  data;		/*  The actual data points		     */
}  alt_data_t;


/*	CACHE_DATA -  Since internal weights do not change in cascor, it is
	possible to store the internal activations and errors from epoch to
	epoch, not having to recalculate every time.  Both values and errors
	are stored for every training point, making a cache expensive, memory-
	wise.
*/

typedef struct  {
  float  **values,	/*  Cached activation values of hidden units	*/
         **errors;	/*  Cached error values of hidden units		*/
}  cache_data;


/*	RUN_RES_T -  Stores the cumulative statistics of the run for the final
	end report.
*/

typedef struct  {
  int     Nvictories,		/*  Number of victories won		     */
          Nepochs,		/*  Average number of epochs until victory   */
          errorBits,		/*  Average number of bits wrong	     */
	  Nunits;		/*  Average number of units in the completed */
				/* network				     */
  float   crossingsSec,		/*  Average number of connection crossings   */
				/* per second				     */
          percentCorrect,	/*  Percent of bits correct		     */
          runTime,		/*  Length of seconds, of the run	     */
          errorIndex,		/*  Average error index			     */
          trueError,		/*  Average true error			     */
          sumSqError;		/*  Average sum squared error		     */
}  run_res_t;


/*	Constant Declarations	*/

#define CONNX				/*  Turn connection crossing	    */
					/* statistics ON		    */

#define MAX_PATH 	40		/*  Maximum pathname length	    */
#define N_EXT		3		/*  Total number of extensions      */
#define DATA_EXT	".data"		/*  Data file extension		    */
#define CONFIG_EXT	".cfg"		/*  Configuration file extension    */
#define WEIGHT_EXT	".net"		/*  Save file extension		    */
#define HELP_FILE	"cascor.hlp"	/*  User help file		    */

#define BIN_POS        0.5		/*  Value for binary '+'	    */
#define BIN_NEG        -0.5		/*  Value for binary '-'	    */

#define DEF_VALIDATE	FALSE		/*  Default value for parm.validate */
#define DEF_TEST	TRUE		/*  Default value for parm.test     */

#define VERSION		"1.1"			    /*  Version number	*/
#define RELEASE_DATE	"5/31/94"		    /*  Release date	*/
#define CONTACT		"neural-bench@cs.cmu.edu"   /*  Site contact at CMU */

/*	Macro to calculate error index	*/

#define ERROR_INDEX( TE, sDev, num )  ( sqrt( TE / num ) / sDev )


#endif
