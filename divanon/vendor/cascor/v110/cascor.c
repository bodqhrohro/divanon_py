/*	Cascade Correlation Learning Algorithm

	v1.1
	Matt White  (mwhite+@cmu.edu)
	5/31/94

	QUESTIONS/COMMENTS: neural-bench@cs.cmu.edu

	This code has been placed in public domain by its author.  As a
	matter of simple courtesy, anyone using or adapting this code is
	expected to acknowledge the source.  The author would like to hear
	about any attempts to use this system, successful or not.

	This code is currently being maintained by the site contact listed
	above.  If you find a bug, add a useful feature, or discover a hack 
        that will increase system performance, please contact the person at 
        the address listed above so that the distribution source may be
	modified accordingly.

	This code is a re-engineered version of the C port, by Scott
	Crowder, of the original Lisp code by Scott Fahlman.  Features have
	been added to allow data sets from the CMU learning benchmark
	database to run on this system.

	For an explanation of this algorithm and some results, see "The
	Cascade-Correlation Learning Architecture" by Scott E. Fahlman and
	and Christian Lebiere in D. S. Touretzky (ed.), "Advances in Neural
	Information Processing Systems 2", Morgan Kaufmann, 1990.  A somewhat
	longer version is available as CMU Computer Science Tech Report
	CMU-CS-90-100, available for FTP from FTP.CS.CMU.EDU in directory
	'/afs/cs/project/connect/tr'.

	RELEASE NOTES

	This is the new version of the older Crowder version of the same
	simulator.  Although we no longer support the Crowder version, it is 
	still available in the 'code/old' directory.

	The major differences between this program and the previous one are
	that we now support the CMU Learning Benchmark Format.  This program 
	accepts both IO and SEQUENCE data files.  Online help is now available 
	at the interaction prompt.  In addition, the output has been cleaned 
	extensively to make it more readable.  If you find a bug (or just have 
	a suggestion) in this code, please send mail to the address above.

	To build this program, unpack the archive and type 'make'.  If you
	desire to use a compiler different from 'cc', add a line that says
	'CC=<compiler>' to the top of the Makefile.  You can also specify 
	optimizations on the 'CFLAGS' line.

	Revision Log
	~~~~~~~~~~~~
	5/31/94         1.1     Added a function to dump results of the test
	                        epoch to a file
	3/30/94         1.0.4   Fixed a bug that caused activation prime for
	                        varsigmoid units in the candidate layer to be
				calculated incorrectly.  Thanks to Hugo Silva
				for pointing this out.
	1/10/94         1.0.3   Fixed a bug that caused validation not to work
	                        correctly in multiple trial runs.
	12/7/93         1.0.2   Fixed a bug which could cause learning
	                        disabilities on 64bit machines.
	10/15/93	1.0.1	Fixed inconsistant entries in ParmTable.
				Fixed bug that caused the last parameter in a
				configuration file not to be read.
				Fixed a bug that caused varied candidates
				not to work.
				Changed call to 'difftime' to a more low level
				form.  Apparently, 'difftime' is not available
				on Sun workstations.
	9/24/93		1.0	Initial release
*/


/*	Include Files	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#include "tools.h"
#include "parse.h"
#include "interface.h"
#include "cascor.h"


/*	Global Variable Declarations	*/

net_data    net;		/*  Internal activation and weights        */
net_info    netConfig;		/*  Information on network topology        */
output_data out;		/*  Information about the network outputs  */
cand_data   cand;		/*  Information about the cadidate units   */
error_data  error;		/*  Network error calculations		   */
parm_data   parm;		/*  Network run parameters		   */
cache_data  cache;		/*  Cached activation and error values	   */
alt_data_t  val,		/*  Validation data set			   */
            test;		/*  Test data set			   */
run_res_t   runResults;		/*  Totalled results from all trials	   */
    
int      Ncand,			/*  Number of candidates being trained	     */
         Nunits,		/*  Number of units in the network	     */
         Ninputs,		/*  Number of inputs to the network	     */
         Noutputs,		/*  Number of outputs from the network       */
         NtrainPts,		/*  Number of training points, minus segment */
				/* markers				     */
         NtrainOutVals,		/*  Number of training points, times the     */
				/* number of outputs			     */
         epoch;			/*  Epoch that has just been calculated      */
unsigned long         connx;	/*  Number of connection crossings	     */
boolean  isSeq,			/*  Is this a sequence data set?	     */
	 dataLoaded,		/*  Has the data been loaded?		     */
         interact,		/*  Interact with the user?		     */
	 helpAvail,		/*  Is help available?			     */
	 interruptPending;	/*  Is there an interrupt pending?	     */


/*	Macro Definitions	*/

/*	RANDOM_WEIGHT -  Returns a random value between plus and minus 'x'  */

#define RANDOM_WEIGHT(x) ( x * (random() % 1000 / 500.0) - x )


/*	NO_CACHE -  Signals insufficient memory and then turns off the cache */

#define NO_CACHE  {							     \
		  fprintf ( stderr, "\nInsufficient memory for cache.\n" );  \
                  fprintf ( stderr, "Shutting cache down.\n" );              \
                  destroy_cache ( );					     \
		  parm.useCache = FALSE;				     \
                  return FALSE;						     \
                  }


/*	Function Prototypes	*/

void      init_prog		( void );	/*  Initialization functions */
void      init_parms		( void );
void      init_vars		( int * );
void      build_net		( int );
void	  init_net		( int );
void	  init_error		( error_data * );
void	  init_cand		( void );

void	  train_outputs		( status_t * );	/*  Output training	*/
void	  output_epoch		( void );	/* functions		*/
void	  adjust_weights	( void );

void	  train_cand		( status_t * );	/*  Correlation machinery  */
void	  correlation_epoch	( void );
void	  cand_epoch		( void );
void	  compute_correlations	( void );
void	  adjust_correlations	( void );
void	  compute_slopes	( void );
void	  adjust_cand_weights	( void );
void	  install_cand		( void );

void	  validation_epoch	( float, int, status_t * );
void	  test_epoch		( void );	/*  Network testing code  */
void      dump_results          ( FILE *, float *, float * );

boolean   build_cache		( int );	/*  Cache code		  */
void      destroy_cache		( void );
void      compute_cache		( void );
void	  recompute_cache	( int );

/*	Miscellanious useful functions	*/

void      forward_pass		( int, int, data_set );
void      setup_inputs		( int, int, data_set );
void      output_pass		( void );
void      compute_error		( float *, error_data *, boolean, boolean );
void	  quickprop		( int, float *, float *, float *, float *, 
                                  float, float, float, float );
float     activation		( int, float );
float     activation_prime	( int, float, float );
float     output_function	( int, float );
float     output_prime		( int, float );
float     std_dev		( data_set, int, int );


void  main  ( int argc, char *argv [] )
{
  int          trial,				/*  Current trial  	     */
               maxUnits;			/*  Maximum units in network */
  time_t       startTime;			/*  Time trial was started   */
  status_t     status    	= TRAINING,	/*  Training status	     */
	       valStat		= TRAINING,	/*  Validation status	     */
               candStat;			/*  Candidate training	     */
						/* status		     */

  /*  Identify the program, load data and parameters, check for user	*/
  /* changes and then initialize the data structures			*/

  prog_id	     ( );  
  init_prog          ( );
  init_parms         ( );
  exec_command_line  ( argc, argv );
  if  ( interact )
    change_parms ( FALSE );
  else
    list_parms  ( );
  init_vars          ( &maxUnits );
  build_net          ( maxUnits );

  /*  Repeatedly run trials until the max is reached  */

  for  ( trial = 0 ; trial < parm.Ntrials ; trial++ )  {

    /*  Initialize for this trial  */

    valStat = TRAINING;
    init_net      ( maxUnits);
    output_begin_trial  ( trial + 1, &startTime );
    if  ( parm.useCache )
      compute_cache  ( );

    /*  Keep training until the network reaches it's maximum size  */

    while  ( Nunits < maxUnits )  {

      /*  Train the outputs until they stagnate, time out or you win  */

      train_outputs ( &status );
      output_train_results  ( status );
      if  ( status == WIN )
        break;

      /*  Run validation epoch  */

      if  ( parm.validate )  {
        validation_epoch ( 0.49999, maxUnits, &valStat );
        if  ( valStat != TRAINING )
          break;
      }

      /*  Train a pool of candidate units and add the best one to the net  */

      init_cand     ( );
      train_cand    ( &candStat );
      install_cand  ( );
      output_cand_results  ( candStat );
    }

    /*  If we have a candidate with untrained outputs, train them  */

    if  ( (status != WIN) && (valStat == TRAINING) )
      train_outputs  ( &status );

    /*  Run test epoch  */

    if  ( parm.test )  
      test_epoch  ( );

    /*  Output the results of this trial  */

    output_trial_results ( status, trial + 1, startTime );
    if  ( parm.saveWeights )
      save_weights ( parm.weightFile, interact, trial + 1, startTime );
  }

  output_run_results ( );
  close_help ( );
}


/********************* Initialization Functions	******************************/

/*	INIT_PROG -  Initializes the random number generator and resets the
	'dataLoaded' flag.
*/

void  init_prog  ( void )
{
  time_t  timer;	/*  Used as a random number seed	*/

  time     ( &timer );	/*  Seed the random number generator	*/
  srandom  ( timer );

  dataLoaded  = FALSE;	/*  Reset the data loaded flag		*/
  interact    = FALSE;	/*  Reset the interaction flag		*/

  helpAvail = init_help ( HELP_FILE );

  interruptPending = FALSE;
  signal  ( SIGINT, trap_ctrl_c );
}


/*	INIT_PARMS -  Defaults all run parameters.  These parameters should
	do OK on two-spirals.
*/

void  init_parms  ( void )
{
  parm.Ntrials     		= 1;	/*  Initialize the integers	     */
  parm.maxNewUnits 		= 25;
  parm.valPatience 		= 12;
  parm.winRadius		= 3;

  parm.weightRange		= 1.0;	/*  Initialize the floating point    */
  parm.weightMult		= 1.0;	/* numbers			     */
  parm.sigPrimeOffset		= 0.1;
  parm.bias			= 1.0;

  parm.dataFile			= NULL;	/*  Initialize the character strings */
  parm.weightFile		= NULL;

  parm.validate			= DEF_VALIDATE;	/*  Initialize boolean	     */
  parm.test			= DEF_TEST;	/* variables		     */
  parm.useCache			= TRUE;
  parm.saveWeights		= FALSE;

  parm.candNewType		= SIGMOID;	/*  Initialize new unit type */

  parm.out.epochs		= 200;		/*  Initialize the output    */
  parm.out.patience		= 12;		/* parameters		     */
  parm.out.sigMax		= BIN_POS;
  parm.out.sigMin		= BIN_NEG;
  parm.out.epsilon		= 1.0;
  parm.out.decay		= 0.0;
  parm.out.mu			= 2.0;
  parm.out.changeThresh		= 0.01;

  parm.cand.epochs		= 200;		/*  Initialize the candidate */
  parm.cand.patience		= 12;		/* parameters		     */
  parm.cand.sigMax		= BIN_POS;
  parm.cand.sigMin		= BIN_NEG;
  parm.cand.epsilon		= 100.0;
  parm.cand.decay		= 0.0;
  parm.cand.mu			= 2.0;
  parm.cand.changeThresh	= 0.03;

  error.indexThresh		= 0.2;		/*  Initialize the error     */
  error.scoreThresh		= 0.4;		/* parameters		     */

  Ncand				= 8;		/*  Initialize the number of */
						/* candidates		     */
}


/*	INIT_VARS -  This is a second pass at initializing variables.
	Variables initialized here are those that require information stored in
	other variables to calculate.  Thus, we put this off until
	configuration is finalized.
*/

void init_vars	( int *maxUnits )
{
  /*  Calculate shrink factor and scaled epsilon for outputs and shrink  */
  /* factor for the candidates						 */

  out.shrinkFactor	= parm.out.mu / (1.0 + parm.out.mu);
  out.scaledEpsilon	= parm.out.epsilon / (netConfig.train_pts -
                                              netConfig.train_seg);
  cand.shrinkFactor 	= parm.cand.mu / (1.0 + parm.cand.mu);

  /*  Calculate standard deviation for all data sets being used		*/

  error.stdDev		= std_dev( netConfig.train, netConfig.train_pts,
                                   NtrainOutVals );

  if  ( parm.validate )
    val.stdDev	= std_dev ( val.data, val.Npts, val.NoutVals );
  if  ( parm.test )
    test.stdDev	= std_dev ( test.data, test.Npts, test.NoutVals );

  runResults.Nvictories		= 0;	/*  Reset run results for this run  */
  runResults.Nepochs		= 0;
  runResults.errorBits		= 0;
  runResults.Nunits		= 0;

  runResults.crossingsSec	= 0.0;
  runResults.percentCorrect	= 0.0;
  runResults.runTime		= 0.0;
  runResults.errorIndex		= 0.0;
  runResults.trueError		= 0.0;
  runResults.sumSqError		= 0.0;
  
  /*  Calculate the total number of units possible  */

  *maxUnits 		= 1 + Ninputs + parm.maxNewUnits;
}


/*	BUILD_NET -  Allocate memory for all network data structures not yet
	allocated.  Allocation assumes that the network can grow to its
	maximum size.
*/
  
void  build_net  ( int maxUnits )
{
  char *fn 	= "Build Net";	/*  Function Identifier	*/
  int  i;			/*  Indexing variable	*/

  if  ( parm.useCache )		/*  Build the cache	*/
    build_cache ( maxUnits );

  /*  Allocate memory for internal activation and weights	*/

  if  ( !( parm.useCache ) )
    net.values  = (float *)     alloc_mem ( maxUnits, sizeof( float ), fn );
  net.weights   = (float **)    alloc_mem ( maxUnits, sizeof( float * ), fn );
  net.unitTypes = (unit_type *) alloc_mem ( maxUnits, sizeof( unit_type ),fn );
  for  ( i = Ninputs + 1 ; i < maxUnits ; i++ )
    net.weights [i] = (float *) alloc_mem ( i, sizeof( float ), fn );

  /*  Allocate memory for the outputs				*/

  out.values   = (float *)  alloc_mem ( Noutputs, sizeof( float ), fn );
  out.weights  = (float **) alloc_mem ( Noutputs, sizeof( float * ), fn );
  out.deltas   = (float **) alloc_mem ( Noutputs, sizeof( float * ), fn );
  out.slopes   = (float **) alloc_mem ( Noutputs, sizeof( float * ), fn );
  out.pSlopes  = (float **) alloc_mem ( Noutputs, sizeof( float * ), fn );
  for  ( i = 0 ; i < Noutputs ; i++ )  {
    out.weights [i] = (float *) alloc_mem ( maxUnits, sizeof( float ), fn );
    out.deltas [i]  = (float *) alloc_mem ( maxUnits, sizeof( float ), fn );
    out.slopes [i]  = (float *) alloc_mem ( maxUnits, sizeof( float ), fn );
    out.pSlopes [i] = (float *) alloc_mem ( maxUnits, sizeof( float ), fn );
  }

  /*  Allocate memory for the candidate units			*/

  cand.values    = (float *)     alloc_mem ( Ncand, sizeof( float ), fn );
  cand.sumVals   = (float *)     alloc_mem ( Ncand, sizeof( float ), fn );
  cand.types     = (unit_type *) alloc_mem ( Ncand, sizeof( unit_type ), fn );
  cand.weights   = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  cand.corr      = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  cand.pCorr     = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  cand.deltas    = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  cand.slopes    = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  cand.pSlopes   = (float **) alloc_mem ( Ncand, sizeof( float * ), fn );
  for  ( i = 0 ; i < Ncand ; i++ )  {
    cand.weights [i] = (float *) alloc_mem( maxUnits, sizeof( float ), fn );
    cand.corr [i]    = (float *) alloc_mem( Noutputs, sizeof( float ), fn );
    cand.pCorr [i]   = (float *) alloc_mem( Noutputs, sizeof( float ), fn );
    cand.deltas [i]  = (float *) alloc_mem( maxUnits, sizeof( float ), fn );
    cand.slopes [i]  = (float *) alloc_mem( maxUnits, sizeof( float ), fn );
    cand.pSlopes [i] = (float *) alloc_mem( maxUnits, sizeof( float ), fn );
  }

  /*  Allocate memory for error structure			*/

  if  ( !(parm.useCache ) )
    error.errors = (float *) alloc_mem ( Noutputs, sizeof( float ), fn );
  error.sumErr   = (float *) alloc_mem ( Noutputs, sizeof( float ), fn );

  /*  Allocate memory for validation connections		*/

  if  ( parm.validate )  {
    val.bestOutConn = (float **)alloc_mem ( Noutputs, sizeof( float * ), fn );
    for  ( i = 0 ; i < Noutputs ; i++ )
      val.bestOutConn [i] = (float *)alloc_mem(maxUnits, sizeof( float ), fn);
  }
}


/*	INIT_NET -  Initializes the vital data structures for the network,
	preparing it for another trial.
*/

void  init_net  ( int maxUnits )
{
  int i,j;	/*  Indexing variables  */

  /*  Initialize the outputs	*/

  for  ( i = 0 ; i < Noutputs ; i++ )  {
    for  ( j = 0 ; j <= Ninputs ; j++ )  {
      out.weights [i][j]	= RANDOM_WEIGHT( parm.weightRange );
      out.deltas [i][j]		= 0.0;
      out.slopes [i][j]		= 0.0;
      out.pSlopes [i][j]	= 0.0;
    }
    for  ( j = Ninputs + 1 ; j < maxUnits ; j++ )  {
      out.deltas [i][j]     = 0.0;
      out.slopes [i][j]     = 0.0;
      out.pSlopes [i][j]    = 0.0;
    }
  } 

  /*  Initialize the cache	*/

  if  ( parm.useCache )
    for  ( i = 0 ; i < netConfig.train_pts ; i++ )
      if  ( netConfig.train [i].inputs != NULL )  {
        for  ( j = 0 ; j < maxUnits ; j++ )
          cache.values [i][j] = 0.0;
        for  ( j = 0 ; j < Noutputs ; j++ )
          cache.errors [i][j] = 0.0;
      }
  
  /*  Initialize the global variables	*/

  Nunits = Ninputs + 1;
  epoch  = 0;
#ifdef CONNX
  connx = 0;
#endif
}


/*	INIT_ERROR -  This function resets the values contained in the data
	structure specified to their starting values
*/

void  init_error  ( error_data *error )
{
  int i;	/*  Indexing variable	*/

  error -> bits		= 0;
  error -> trueErr	= 0.0;
  error -> sumSqErr	= 0.0;
  for  ( i = 0 ; i < Noutputs ; i++ )
    error -> sumErr [i]	= 0.0;
}


/*	INIT_CAND -  Sets up a new pool of candidates to be trained for
	insertion into the network.  Determines the type for each of these new
	units.
*/

void  init_cand  ( void )
{
  int i,j;	/*  Indexing variables	*/

  /*  Reset the candidate values	*/

  for  ( i = 0 ; i < Ncand ; i++ )  {
    cand.values  [i] = 0.0;
    cand.sumVals [i] = 0.0;
    for  ( j = 0 ; j < Noutputs ; j++ )  {
      cand.corr [i][j]     = 0.0;
      cand.pCorr [i][j] = 0.0;
    }
    for  ( j = 0 ; j < Nunits ; j++ )  {
      cand.weights [i][j]    = RANDOM_WEIGHT( parm.weightRange );
      cand.deltas  [i][j]    = 0.0;
      cand.slopes  [i][j]    = 0.0;
      cand.pSlopes [i][j]    = 0.0;
    }

    /*  Select a type for the new unit.  This is either candNewType, or a  */
    /* mix, if candNewType is set to VARIED				   */

    if  ( parm.candNewType == VARIED )
      switch ( i % 4 )  {
        case 0:  cand.types [i] = SIGMOID;
	         break;
	case 1:  cand.types [i] = ASIGMOID;
	         break;
	case 2:  cand.types [i] = GAUSSIAN;
	         break;
	case 3:  cand.types [i] = VARSIGMOID;
                 break;
	}
    else
      cand.types [i] = parm.candNewType;
  }
}


/********************** Output Training Functions ****************************/

/*	TRAIN_OUTPUTS -  Train the outputs for a number of epochs, until 
        victory, the maximum epoch set in the parameters is reached, or error 
	ceases to improve measureably.  Return the final result in *status.
	Victory (WIN) is achieved when either error bits goes to zero (if
	measure = BITS), or error index falls below indexThresh (if measure =
	INDEX).  Stagnation (STAGNANT) occurs when we go 'patience' epochs
	without significant improvement.  Time Out (TIMEOUT) occurs when we
	go through 'out.epochs' epochs without either victory or stagnation.
*/

void  train_outputs  ( status_t *status )
{
  int     quitEpoch = 0,	/*  Epoch that training is considered	*/
				/* stagnant				*/
          i;			/*  Indexing variable			*/
  float   lastError;		/*  This is the True Error to beat	*/

  for  ( i = 0 ; i < parm.out.epochs ; i++ )  {
    init_error ( &error );	/*  Initialize the error		     */

    output_epoch ( );		/*  Perform a training epoch on the outputs  */
    
    check_interrupt ( );	/*  Check for user interrupt		     */

    /*  Check to see if victory was achieved.  If so, set status and return  */

    if  ( (error.measure == BITS) && (error.bits == 0) )  {
      *status = WIN;
      return;
    }  else  
    if  (error.measure == INDEX)  {
      error.index = ERROR_INDEX( error.trueErr, error.stdDev, NtrainOutVals );
      if  ( error.index <= error.indexThresh )  {
        *status = WIN;
        return;
      }
    }

    /*  If victory was not achieved, adjust the output weights and increment */
    /* the epoch coounter						     */

    adjust_weights  ( );
    epoch++;

    /*  Check for appreciable change in error.  If no change is detected,    */
    /* check for stagnation						     */

    if  ( i == 0 )
      lastError = error.trueErr;
    else
    if  ( fabs( error.trueErr - lastError ) > 
              ( lastError * parm.out.changeThresh) )  {
      lastError = error.trueErr;
      quitEpoch = epoch + parm.out.patience;
    }  else
    if  ( epoch == quitEpoch )  {
      *status = STAGNANT;
      return;
    }
  }

  *status = TIMEOUT;	/*  If haven't left by now, we must have timed out  */
}


/*	OUTPUT_EPOCH -  Perform forward propogation and error computation on
	each of the training points in the training set.
*/

void output_epoch  ( void )
{
  int  offsetLeft,	/*  Number of points until an output is expected  */
       i;		/*  Indexing variable				  */

  offsetLeft = netConfig.offset;	/*  Reset the offset  */

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )

    /*  Check for a segment marker  */

    if  ( netConfig.train [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {

	/*  Use cached values if the cache is on  */

        if  ( parm.useCache )  {
          net.values    = cache.values [i];
          error.errors  = cache.errors [i];   
          output_pass ( );
        }  else
          forward_pass ( i, netConfig.train_pts, netConfig.train );

	/*  Compute error for this presentation  */

        compute_error ( netConfig.train [i].outputs, &error, TRUE, TRUE );

      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;
}


/*	ADJUST_WEIGHTS -  Go through and use quickprop to adjust each of the
	output weights.  Uses slopes, previous slopes, and delta values
	computed in compute_error and in the last weight update.
*/

void  adjust_weights  ( void )
{
  float *ow,	/*  Output weights  */
	*od,	/*  Output deltas   */
	*os,	/*  Output slopes   */
	*op;	/*  Output pSlopes  */
  int   i,j;	/*  Indexing variables	*/

  /*  Update each of the output weights	*/

  for  ( i = 0 ; i < Noutputs ; i++ )  {
    ow = out.weights [i];	/*  Simple speed hack to keep from  */
    od = out.deltas [i];	/* recomputing array locations      */
    os = out.slopes [i];
    op = out.pSlopes [i];
    for  ( j = 0 ; j < Nunits ; j++ )
      quickprop  ( j, ow, od, os, op, out.scaledEpsilon, parm.out.decay, 
                   parm.out.mu, out.shrinkFactor ); 
  }
}


/********************** Candidate Training Code ******************************/

/*	TRAIN_CAND -  Train a new pool of candidates.  Training contiues until
	either the maximum number of training epochs for a candidate pool has
	been reached (TIMEOUT), or a specified number of epochs passes without
	noticeable improvement (STAGNANT).  Victory (WIN) is not possible at
	this juncture.  Training results are returned in *status, so that they
	may be reported to the user.

	Note:  Ideally, after each adjustment of the candidate weights, we
	would run two epochs.  The first would just determine the correlations
	between the candidate unit outputs and the residual error.  Then in a
	second pass, we would adjust each candidate's input weights so as to
	maximize the absolute value of the correlation.  We need to know the
	direction to tune the input weights.

	Since this ideal method doubles the number of epochs required for
	training the candidate, we cheat slightly and use the correlation
	values computed BEFORE the most recent weight update.  This combines
	the two epochs, saving us almost a factor of two.  To bootstrap the
	process, we begin with a single epoch that computes only the
	correlation.

	Since we look only at the sign of the correlation after the first ideal
	epoch and since that sign should change very infrequently, this
	probably is OK.   But keep a lookout for pathelogical situations in
	which this might cause oscillation.
*/

void  train_cand  ( status_t *status )
{
  float     lastScore = 0.0;	/*  This is the correlation score to beat   */
  int       quitEpoch = 0,	/*  This is the epoch that we should quit   */
				/* if we don't beat lastScore		    */
            i, j;		/*  Indexing variables			    */

  for  ( i = 0 ; i < Noutputs ; i++ )	/*  Average the sum errors over all  */
    error.sumErr [i]  /= NtrainPts;	/* the training patterns	     */

  correlation_epoch  ( );	/*  Calculate initial correlation values  */

  check_interrupt  ( );		/*  Check for user interrupt		  */

  for  ( i = 0 ; i < parm.cand.epochs ; i++ )  {
    cand_epoch  ( );		/*  Train the candidates for an epoch,  */
				/* recomputing correlations as you go   */

    check_interrupt  ( );	/*  Check for user interrupt		*/

    adjust_cand_weights  ( );	/*  Adjust the connections leading into the  */
				/* candidates using quickprop		     */
    adjust_correlations  ( );	/*  Adjust the correlation values for the    */
				/* candidates.				     */

    epoch++;			/*  Increment the epoch counter		     */

    /*  This is the stagnation machinery.  Basically the same as             */
    /* train_outputs, except that it keys off of correlation values instead  */
    /* of True Error.  The first 'if' statement runs on the first pass       */
    /* through the loop and serves to bootstrap the rest of the process	     */
    /* The second conditional checks for improvement in correlation values   */
    /* while the third checks for the actual stagnation			     */

    if  ( i == 0 )
      lastScore = cand.bestScore;
    else
    if  ( fabs( cand.bestScore - lastScore ) > 
          ( lastScore * parm.cand.changeThresh ) )  {
      quitEpoch = epoch + parm.cand.patience;
      lastScore = cand.bestScore;
    } else
    if  ( epoch == quitEpoch )  {
      *status = STAGNANT;
      return;
    }
  }

  *status = TIMEOUT;	/*  If we haven't left by now, we've timed out  */
}


/*	CORRELATION_EPOCH -  Run a single epoch to compute correlation values
	only.  After this epoch, we will compute correlation values during
	training.  This is a slight deviation from the 'ideal' candidate
	training cycle that saves almost half the epochs.  For more
	information, see the notes at the end of the notes for TRAIN_CAND.
*/

void correlation_epoch  ( void )
{
  int offsetLeft,	/*  Number of pattern presentations left until an  */
			/* output is expected				   */
      i;		/*  Indexing variable				   */

  offsetLeft = netConfig.offset;	/*  Reset offsetLeft		   */

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )
    if  ( netConfig.train [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {

	/*  Either pull the activation values from the cache, if its on, or  */
	/* recompute them from scratch if it is off			     */

        if  ( parm.useCache )  {
          net.values   = cache.values [i];
          error.errors = cache.errors [i];
        }  else  {
          forward_pass  ( i, netConfig.train_pts, netConfig.train );
          compute_error ( netConfig.train [i].outputs, &error, FALSE, FALSE );
        } 

        compute_correlations  ( );	/*  Compute correlation values for  */
					/* this training point		    */
      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;

  adjust_correlations ( );	/*  Normalize the correlations and then  */
  epoch++;			/* update  the epoch counter		 */
}


/*	CAND_EPOCH -  Train the candidates for an epoch.  If the cache is not
	on, run a forward pass and compute error.  Otherwise, retrieve
	activation values from the cache.  Use these values to calculate the
	slopes and correlation values of each of the candidates.
*/

void cand_epoch  ( void )
{
  int offsetLeft,	/*  Number of inputs to read without output  */
      i;		/*  Indexing variable			     */


  offsetLeft = netConfig.offset;	/*  Reset the offset  */

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )
    if  ( netConfig.train [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {

	/*  Get the activation values and errors	*/

        if  ( parm.useCache )  {
          net.values    = cache.values [i];
          error.errors  = cache.errors [i];
        }  else  {
          forward_pass  ( i, netConfig.train_pts, netConfig.train );
          compute_error ( netConfig.train [i].outputs, &error, FALSE, FALSE );
        }

        compute_slopes  ( );	/*  Compute slope and correlation values  */

      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;
}


/*	COMPUTE_CORRELATIONS -  For the current training pattern, compute the
	activation of each candidate unit.  Then begin to compute the
	correlation value for that unit.  Activation values and error from the
	rest of the network have already been computed elsewhere.
*/

void compute_correlations  ( void )
{
  float sum,		/*  Sum of input stimulus to this unit   	    */
        val,		/*  Activation value of this unit	 	    */
        *cWghts,	/*  Pointer to this candidate's weights  	    */
	*cCorr;		/*  Pointer to this candidate's correlation values  */
  int   i, j;		/*  Indexing variables				    */
  
  for  ( i = 0 ; i < Ncand ; i++ )  {
    sum 	= 0.0;			/*  Reset Sum  */
    cWghts    	= cand.weights [i];	/*  Used in a simple speed hack  */
    cCorr	= cand.corr [i];

    for  ( j = 0 ; j < Nunits ; j++ )		/*  Compute sum of this cand */
      sum += cWghts [j] * net.values [j];

    /*  Compute the activation value of this unit	*/

    val              =  activation( cand.types [i], sum );
    cand.values [i]  =  val;
    cand.sumVals [i] += val;

    /*  Compute the correlation for this unit	*/

    for  ( j = 0 ; j < Noutputs ; j++ )
      cCorr[j] += val * error.errors [j];
  }
}


/*	ADJUST_CORRELATIONS -  Normalize each candidate's correlation score
	and then stuff that value into the previous correlation structure.
	Zero out the correlation score to prepare for the next round.  Note
	which unit has the best total score to date.
*/

void  adjust_correlations  ( void )
{
  float avgValue,	/*  Average value of this candidate over all	   */
			/* training points				   */
        score,		/*  Sum of the absolute values of the corelations  */
        cor,		/*  Correlation of this candidate to this output   */
        *curCor,	/*  Pointer to help improve speed		   */
        *prevCor;
  int   i,j;		/*  Indexing variable				   */

  cand.best      = 0;	/*  Reset pointers to the best units		   */
  cand.bestScore = 0.0;

  for  ( i = 0 ; i < Ncand ; i++ )  {
    avgValue =  cand.sumVals [i] / NtrainPts;	/*  Calculate  avgValue	*/  

    score    = 0.0;		/*  Reset score counter	*/
    curCor   = cand.corr  [i];	/*  Set pointers to array index being worked */
    prevCor  = cand.pCorr [i];	/* on, this saves costly array operations    */

    /*  Calculate the correlation value for each of the outputs and then     */
    /* update the candidate's data structures.  Finally update this	     */
    /* candidate's score						     */

    for  ( j = 0 ; j < Noutputs ; j++ )   {
      cor          = ( curCor [j] - avgValue * error.sumErr [j] ) 
                     / error.sumSqErr;
      prevCor [j]  = cor;
      curCor  [j]  = 0.0;
      score        += fabs( cor );
    }

    cand.sumVals [i] = 0.0;

    /*  See if this is the best candidate so far  */

    if  ( score > cand.bestScore )  {
      cand.bestScore = score;
      cand.best      = i;
    }
  }
}


/*	COMPUTE_SLOPES -  Use the precomputed correlation values to compute
	the slopes of the error for each individual candidate unit.  This will
	later be used to update the input weights to each of these candidates.

	Note:  This function is extremely compute-intensive.  If you want to
	spend some time optimizing, this is a good place to start.  I think
	I've done everything reasonable, but if you find something, please
	email me at 'mwhite+@cmu.edu', so that I can modify the release
	version.
*/

void  compute_slopes  ( void )
{
  float sum,		/*  Sum of stimulus coming into a candidate  	   */
        change,		/*  Used to compute both the slope and the corr    */
        value,		/*  The activation value of this candidate         */
        actPrime,	/*  Activation prime value for this candidate      */
        err,		/*  Raw error for this candidate for this pattern  */
        direction,	/*  Direction to adjust weights to improve corr    */
        *cWghts,	/*  These four pointers are used to help improve   */
        *cCorr,		/* speed					   */
        *cPCorr,
        *cSlp;
  int   i, j;		/*  Indexing variables				   */

  for  ( i = 0 ; i < Ncand ; i++ )  {
    sum    = 0.0;		/*  Initialize to variables for this	*/
    change = 0.0;		/* candidate				*/
    cWghts = cand.weights [i];
    cCorr  = cand.corr [i];
    cPCorr = cand.pCorr [i];
    cSlp   = cand.slopes [i];

    for  ( j = 0 ; j < Nunits ; j++ )		/*  Compute stimulus for  */
      sum += net.values [j] * cWghts [j];	/* this candidate	  */

    /*  Compute activation and activation prime for this candidate  */

    value              =  activation( cand.types [i], sum );
    actPrime           =  activation_prime( cand.types [i], value, sum );
    cand.sumVals [i]   += value;

    /*  Normalize activation prime by the sum squared error  */

    actPrime		/= error.sumSqErr;

    /*  Compute the correlation to each of the outputs   */

    for  ( j = 0 ; j < Noutputs ; j++ ) {
      err            =  error.errors [j];
      direction      =  ( cPCorr [j] < 0.0 ) ? -1.0 : 1.0;
      change         -= direction * actPrime * ( err - error.sumErr [j] );
      cCorr [j]      += err * value;
    }

    /*  Use the 'change' value just computed to compute the slopes for all  */
    /* the weights feeding into this candidate				    */

    for  ( j = 0 ; j < Nunits ; j++ )
      cSlp [j] += change * net.values [j];
  }
}


/*	ADJUST_CAND_WEIGHTS -  Using the slopes and deltas previously computed,
	use quickprop to update each the weights feeding into each of the
	candidates.
*/

void adjust_cand_weights  ( void )
{
  float scaledEpsilon,	/*  Epsilon scaled by the number of training points  */
			/* times the current number of units in the network  */
	*cw,		/*  These four pointers are used to cut down on      */
	*cd,		/* array operations, thus speeding things up in here */
	*cs,
	*cp;
  int   i, j;		/*  Indexing variables				     */

  /*  We must scale the epsilon locally, since the scaling factor is going   */
  /* to change on every cycle through the network			     */

  scaledEpsilon = parm.cand.epsilon / (float) ( NtrainPts * Nunits );

  /*  Perform a quickprop update on each of the weights  */

  for  ( i = 0 ; i < Ncand ; i++ )  {
    cw = cand.weights [i];	/*  Set the pointers to their positions in  */
    cd = cand.deltas [i];	/* the arrays				    */
    cs = cand.slopes [i];
    cp = cand.pSlopes [i];
    for  ( j = 0 ; j < Nunits ; j++ )
      quickprop  ( j, cw, cd, cs, cp, scaledEpsilon, parm.cand.decay, 
                   parm.cand.mu, cand.shrinkFactor );
  }
}
    

/*	INSTALL_CAND -  Take the best candidate unit and install it in the
	network.  This is accomplished by copying its weights and unit type
	over into the 'net' data structure.  If the cache is in use, it is
	now necessary to recompute cache values so that the new unit is taken
	into consideration.
*/

void install_cand  ( void )
{
  float *newWeights,		/*  Pointer to the actual weight array       */
        *candBestWeights,	/*  Pointer to the candidate's weight array  */
        weightModifier;		/*  Scaling factor for new output weights    */
  int   i;			/*  Indexing variable			     */

  /*  Set up pointers to the appropriate weight arrays  */

  newWeights      = net.weights [Nunits];
  candBestWeights = cand.weights [cand.best];

  /*  Copy the weights over  */

  for  ( i = 0 ; i < Nunits ; i++ ) 
    newWeights [i] = candBestWeights [i];

  /*  Use the correlation score to each output as a starting point for the  */
  /* new output weights.  Modify this value by the weight multiplier.  If   */
  /* we are working with an error index, scale this value by the number of  */
  /* active units in the network.					    */

  if  ( error.measure == BITS )
    weightModifier = parm.weightMult;
  else
    weightModifier = parm.weightMult / Nunits;

  for  ( i = 0 ; i < Noutputs ; i++ )
    out.weights [i][Nunits] = -cand.pCorr [cand.best][i] * weightModifier;

  net.unitTypes [Nunits] = cand.types [cand.best];

  if  ( parm.useCache )		/*  Recompute cache values for the new unit  */
    recompute_cache  ( Nunits );

  Nunits++;	/*  Let the rest of the network know about the new unit  */
}


/*********************** Generalization Code *********************************/

/*	VALIDATION_EPOCH -  Run a complete validation epoch.  No training is
	performed, only a measurement of error.  If a certain number of epochs,
	measured by valPatience, passes without improvement, restore the net to
	the point of its best generalization.
*/

void  validation_epoch  ( float testThreshold, int maxUnits, status_t *status )
{
  static float		*vVals;		/*  Dummy activation values array  */
  float			*vErr,		/*  Dummy error array		   */
                     	*vSumErr,	/*  Dummy sum of errors array	   */
  			*oldVals;	/*  Pointer back to orginal vals   */
  int                   offsetLeft,	/*  Number of inputs left with no  */
					/* outputs			   */
			i,j;		/*  Indexing variables		   */
  static boolean	firstTime = TRUE;	/*  First val. epoch?	   */
  char		     	*fn = "Validation Epoch";	/*  Function ID	   */
  static error_data	valErr;		/*  Dummy error structure for val  */

  /*  If this is the first validation epoch, allocate memory for the arrays  */
  /* that will be needed for these computations			   	     */

  if  ( firstTime )  {
    vErr    = (float *) alloc_mem  ( Noutputs, sizeof( float ), fn );
    vSumErr = (float *) alloc_mem  ( Noutputs, sizeof( float ), fn );
    vVals   = (float *) alloc_mem  ( maxUnits, sizeof( float ), fn );

    valErr.errors	= vErr;
    valErr.sumErr	= vSumErr;
  }

  oldVals		= net.values;	/*  Swap the values in net with the  */
  net.values		= vVals;	/* dummy activation values	     */

  init_error  ( &valErr );		/*  Reset the error values	     */

  offsetLeft = netConfig.offset;	/*  Reset the offset		     */


  /*  Compute an epoch with the cache off.  Measure the error		     */

  for  ( i = 0 ; i < val.Npts ; i++ )
    if  ( val.data [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {
        forward_pass  ( i, val.Npts, val.data );
        compute_error ( val.data [i].outputs, &valErr, TRUE, FALSE );
      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;

  check_interrupt  ( );

  /*  Compute the error index for this validation epoch		 	     */

  if  ( error.measure == INDEX )
    valErr.index = ERROR_INDEX( valErr.trueErr, val.stdDev, val.NoutVals );

  /*  Keep track of the best error values to date and the connections that   */
  /* go along with them.  If generalization ceases to improve, restore the   */
  /* network to the state of its best performance.  This is accomplished by  */
  /* setting the 'Nunits' value back down to only count the helpful units    */
  /* and by copying the connections between these units and the outputs back */
  /* into the 'out' structure.	Setting a status flag informs the calling    */
  /* function that training has stagnated and that it should end.	     */

  if  ( firstTime )  {
    firstTime     = FALSE;
    val.bestScore = valErr.trueErr;
    val.bestPass  = Nunits;
  }  else
  if  ( valErr.trueErr < val.bestScore )  {
    val.bestScore = valErr.trueErr;
    val.bestPass  = Nunits;
    for  ( i = 0 ; i < Noutputs ; i++ )
      for  ( j = 0 ; j < Nunits ; j++ )
        val.bestOutConn [i][j] = out.weights [i][j];
  }  else
  if  ( (Nunits - val.bestPass) > parm.valPatience )  {
    Nunits = val.bestPass;
    for  ( i = 0 ; i < Noutputs ; i++ )
      for  ( j = 0 ; j < Nunits ; j++ )
        out.weights [i][j] = val.bestOutConn [i][j];
    *status = STAGNANT;
    firstTime = TRUE;
  }

  net.values	= oldVals;		/*  Restore the internal activation  */
					/* array to its origonal state	     */

  output_val_results ( valErr );	/*  Output validation results to the */
					/* user				     */
}


/*	TEST_EPOCH -  Run one final epoch over an alternate data set, to see if
	we have generalized this problem well.
*/

void  test_epoch  ( void )
{
  int  offsetLeft,	/*  Number of inputs until output  */
       i;  		/*  Indexing variable		   */
  FILE *fptr;            /*  Pointer to dump file for test outputs  */

  if  ( (fptr=fopen("test.results","w")) == NULL )  {
    fprintf (stderr,"ERROR: Unable to open file 'test.results' for writing");
    exit( 1 );
  }

  offsetLeft = netConfig.offset;

  init_error ( &error );

  for  ( i = 0 ; i < test.Npts ; i++ )
    if  ( test.data [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {
        forward_pass  ( i, test.Npts, test.data );
	dump_results  ( fptr, test.data[i].inputs, test.data[i].outputs );
        compute_error ( test.data [i].outputs, &error, TRUE, FALSE );
      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;

  fclose( fptr );

  check_interrupt  ( );
}

void dump_results  ( FILE *fptr, float *inputs, float *outputs )
{
  int i;

  for  ( i = 0; i < Ninputs-1; i++)
    fprintf (fptr,"%f, ",inputs[i]);
  fprintf (fptr,"%f  =>  ",inputs[Ninputs-1]);
  for  ( i = 0; i < Noutputs-1; i++)
    fprintf (fptr,"%f (%f), ",outputs[i],out.values[i]);
  fprintf (fptr,"%f (%f);\n",outputs[i],out.values[i]);
}

/************************ Cache Routines *************************************/

/*	BUILD_CACHE -  Allocate memory for the cache.  If, at any point, we run
	out of memory, dismantle the cache and return a FALSE value to the
	calling function, so that it knows to turn off the cache.
*/

boolean build_cache  ( int maxUnits )
{
  int i;	/*  Indexing variable  */

  if  ( ( cache.values = (float **) calloc ( netConfig.train_pts,
          sizeof( float * ) )) == NULL )
    NO_CACHE;
  if  ( ( cache.errors = (float **) calloc ( netConfig.train_pts,
          sizeof( float * ) )) == NULL )
    NO_CACHE;

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )  {
    if  ( ( cache.values [i] = (float *) calloc ( maxUnits, 
            sizeof( float ) )) == NULL )
      NO_CACHE;
    if  ( ( cache.errors [i] = (float *) calloc ( Noutputs,
            sizeof( float ) )) == NULL )
      NO_CACHE;
  }

  return TRUE;
}


/*	DESTROY_CACHE -  Go through and deallocate memory for the cache,
	first checking at each point that memory was allocated in the first
	place.
*/

void destroy_cache  ( void )
{
  int i;	/*  Indexing variable	*/

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )  {
    if  ( cache.values [i] != NULL )
      free( cache.values [i] );
    if  ( cache.errors [i] != NULL )
      free( cache.errors [i] );
  }

  if  ( cache.values != NULL )
    free( cache.values );
  if  ( cache.errors != NULL )
    free( cache.errors );

  cache.values = NULL;
  cache.errors = NULL;
}


/*	COMPUTE_CACHE -  Go through and setup all the inputs for the cache.
*/

void  compute_cache  ( void )
{
  int i;

  for  ( i = 0 ; i < netConfig.train_pts ; i++ )
    if  ( netConfig.train [i].inputs != NULL )  {
      net.values = cache.values [i];
      setup_inputs ( i, netConfig.train_pts, netConfig.train );
    }
}


/*	RECOMPUTE_CACHE -  After a new unit has been added to the network, it
	is necessary to compute cache activation values for it.  That's what
	this function does.
*/

void recompute_cache  ( int unit_no )
{
  float sum;	/*  Sum of stimulus for the unit  */
  int i, j;	/*  Indexing variables		  */
  
  for  ( i = 0 ; i < netConfig.train_pts ; i++ )
    if  ( netConfig.train [i].inputs != NULL )  {
      sum = 0.0;

      /*  Compute stimulus for this new unit  */

      for  ( j = 0 ; j < unit_no ; j++ )
        sum += cache.values [i][j] * net.weights [unit_no][j];

      /*  Store activation value in the cache  */

      cache.values [i][unit_no] = activation( net.unitTypes [unit_no], sum );

#ifdef CONNX
      connx += unit_no;
#endif
    }
}


/******************* Miscellanious Network Functions *************************/

/*	FORWARD_PASS -  Forward propogate through the network for a single
	training pattern.  The data set is specifiable through the dataSet
	parameter.  Npts is the number of points in the data set and pattern
	indicates which training pattern to use.
*/

void  forward_pass  ( int pattern, int Npts, data_set dataSet )
{
  float sum,		/*  Sum of stimulus for the unit being calculated   */
        *wghts;		/*  Pointer to the weights being used.  Speed hack  */
  int   i, j;		/*  Indexing variables  */	

  setup_inputs  ( pattern, Npts, dataSet );

  /*  Compute the values of the hidden units  */ 

  for  ( i = Ninputs + 1 ; i < Nunits ; i++ )  {
    sum   = 0.0;
    wghts = net.weights [i];

    /*  Sum the stimulus  */

    for ( j = 0 ; j < i ; j++ )
      sum += net.values [j] * wghts [j];

    /*  Get the activation value  */

    net.values [i] = activation( net.unitTypes [i], sum );

#ifdef CONNX
    connx += i - 1;
#endif
  }

  output_pass  ( );	/*  Compute the values of the outputs  */
}


/*	SETUP_INPUTS -  Setup the input units for forward propogation.  This is
	complicated by the possible presence of sequence data sets.  If this is
	a sequence data set, then a nettalk-like approach is used on it.  The
	size of the window used is user selectable in parm.winRadius.  A '0'
	would mean just look at this pattern, a '1' would mean look at this
	pattern and the ones immediately before and after, and so on.  If a
	segment marker is reached, then all further inputs in that direction
	receive no stimulus.
*/

void  setup_inputs  ( int pattern, int Npts, data_set dataSet )
{
  boolean  nullEnc;	/*  Has a marker been encountered in this direction? */
  int      i, j,	/*  General indexing variables  		     */
           pat;		/*  Indexing variable pointing to location in the    */
			/* the data set.				     */

  net.values [0] = parm.bias;	/*  Setup bias unit  */

  if  ( isSeq )  {
    i       = parm.winRadius;	/*  Initialize local variables  */
    pat     = pattern;
    nullEnc = FALSE;

    /*  Setup the center inputs  */

    for  ( j = 0 ; j < netConfig.inputs ; j++ )
      net.values [(i*netConfig.inputs)+j+1] = dataSet[pat].inputs[j];

    i--;
    pat--;

    /*  Go through and setup the inputs before the center  */

    while ( i >= 0 )  {
      if  ( dataSet[pat].inputs == NULL )
        nullEnc = TRUE;
      if  ( nullEnc || ( pat < 0 ) )
        for  ( j = 0 ; j < netConfig.inputs ; j++ )
          net.values [(i*netConfig.inputs)+j+1] = 0.0;
      else
        for  ( j = 0 ; j < netConfig.inputs ; j++ )
          net.values [(i*netConfig.inputs)+j+1] = dataSet[pat].inputs[j];
      i--;
      pat--;
    }

    /*  Go through and setup the inputs after the center set  */

    i       = parm.winRadius  + 1;
    pat     = pattern + 1;
    nullEnc = FALSE;
    while  ( i <= (2*parm.winRadius + 1) )  {
      if  ( dataSet[pat].inputs == NULL )
        nullEnc = TRUE;
      if  ( nullEnc || ( pat >= Npts ) )
        for  ( j = 0 ; j < netConfig.inputs ; j++ )
          net.values [(i*netConfig.inputs)+j+1] = 0.0;
      else
        for  ( j = 0 ; j < netConfig.inputs ; j++ )
          net.values [(i*netConfig.inputs)+j+1] = dataSet[pat].inputs[j];
      i++;
      pat++;
    }
  } else {

    /*  If this is a standard IO data set, there isn't nearly so much to do  */

    for  ( i = 0 ; i < Ninputs ; i++ )
      net.values [i + 1] = dataSet [pattern].inputs [i];
  }

#ifdef CONNX
  connx += Ninputs + 1;
#endif
}


/*	OUTPUT_PASS -  After doing a forward pass, or retrieving those values
	from the cache, it is still necessary to compute the output values.
*/

void  output_pass  ( void )
{
  float sum,	/*  Sum of stimulus to a specific output  */
        *outW;	/*  Pointer to the output weights	  */
  int   i, j;	/*  Indexing variables			  */

  for  ( i = 0 ; i < Noutputs ; i++ )  {
    sum  = 0.0;			/*  Initialize the local variables  */
    outW = out.weights [i];

    /*  Compute the sum stimulus for this output  */

    for  ( j = 0 ; j < Nunits ; j++ )
      sum += net.values [j] * outW [j];

    /*  Compute the activation value for this output  */

    out.values [i] = output_function( out.types [i], sum );
  }

#ifdef CONNX
  connx += Noutputs * Nunits;
#endif
}


/*	COMPUTE_ERROR -  Computes the error between the actual activation
	values of the outputs and the goal vector.  This error information is
	stored in a data structure 'err', allowing this same function to be
	used in validation epochs as well as training epochs.  The boolean
	variable 'alter_stats' determines whether or not to collect error
	statistics.  'alterSlopes' indicates whether to compute slope values
	for this training point.
*/

void  compute_error  ( float *goal, error_data *err,
		       boolean alter_stats, boolean alterSlopes )
{
  float dif,		/*  The difference between the output being examined */
			/* and it's goal				     */
        error_prime,	/*  The error prime for this output		     */
        val;		/*  The output value being examined		     */
  int   i, j;		/*  Indexing variables				     */

  for  ( i = 0 ; i < Noutputs ; i++ )  {

    /*  Find the difference between output and goal, and then compute error  */
    /* error prime							     */

    val         = out.values [i];
    dif         = val - goal [i];
    error_prime = dif * output_prime( out.types [i], val );

    err -> errors [i]  = error_prime;

    /*  Compute True Error, Sum Squared Error, Sum Error and Error Bits for  */
    /* this output value						     */

    if  ( alter_stats )  {
      if  ( fabs( dif ) > error.scoreThresh )
        err -> bits++;
      err -> trueErr   += dif * dif;
   
      err -> sumErr [i]  += error_prime;
      err -> sumSqErr    += error_prime * error_prime;
    }
    
    /*  Compute the slopes for this output				     */

    if  ( alterSlopes )
      for ( j = 0 ; j < Nunits ; j++ )
        out.slopes [i][j] += error_prime * net.values [j];
  }
}


/*	QUICKPROP -  Perform a weight update on the specified weight using
	Scott Fahlman's QuickProp algorithm.  Weights, deltas, slopes and
	previous slopes are passed to quickprop via pointers to arrays.  The
	specific weight to update is selected via the 'node' parameter.
	Epsilon, decay, mu, and shrink factor are just passed in raw.
*/

void  quickprop  ( int node, float *weights, float *deltas, float *slopes, 
                   float *pSlopes, float epsilon, float decay, float mu, 
                   float shrinkFactor )
{
  float w,		/*  Weight being updated  */
	d,		/*  Delta value for this weight  */
	s,		/*  Slope for this weight  */
	p,		/*  Previous slope for this weight  */
        nextStep = 0.0;	/*  Step to be taken  */

  w = weights [node];			/*  Initialize local variables  */
  s = slopes [node] + decay * w;	/*  Add decay to the slope	*/
  d = deltas [node];
  p = pSlopes [node];

  if  ( d < 0.0 )  {
    if  ( s > 0.0 )		
      nextStep -= epsilon * s;	
				
    if  ( s >= ( shrinkFactor * p ) ) 
      nextStep += mu * d;		
					
    else				
      nextStep += d * s / (p - s);	
  }
  else if  ( d > 0.0 )  {
    if  ( s < 0.0 )			
      nextStep -= epsilon * s;		
    if  ( s <= ( shrinkFactor * p ) )	
      nextStep += mu * d;		
    else				
      nextStep += d * s / (p - s);
  }
  else
    nextStep -= epsilon * s;	/*  Last step was zero, so only use linear   */
				/* term					     */

  deltas [node]  =  nextStep;	/*  Copy information back into the original  */
  weights [node] += nextStep;	/* data structures, and modify the weight    */
  pSlopes [node] =  s;		/* value				     */
  slopes [node]  =  0.0;
}


/*	ACTIVATION -  Compute the activation value of a unit of type 'unitType'
	and stimulus 'sum'.  This is basically just a big case statement.  New
	unit types can easily be added by inserting them here, output_function,
	the two activation prime functions, and in the unit_t data structure.
*/

float activation  ( int unitType, float sum )
{
  float  temp;	/*  Temporary variable to compute gaussian units  */

  switch  ( unitType )  {  
    case SIGMOID    :  if  ( sum < -15.0 )
                         return -0.5;
                       if  ( sum > 15.0 )
                         return 0.5;
                       return  ( 1.0 / (1.0 + exp( -sum )) - 0.5 );
    case ASIGMOID   :  if  ( sum < -15.0 )
                         return 0.0;
                       if  ( sum > 15.0 )
                         return 1.0;
                       return  ( 1.0 / (1.0 + exp( -sum  )) );
    case VARSIGMOID :  if  ( sum < -15.0 )
                         return parm.cand.sigMin;
                       if  ( sum > 15.0 )
                         return parm.cand.sigMax;
                       return  ( (parm.cand.sigMax - parm.cand.sigMin) / 
                                 (1.0 + exp( -sum )) +
                                 parm.cand.sigMin );
    case GAUSSIAN   :  temp = -0.5 * sum * sum;
                       if  ( temp < -75.0 )
                         return 0.0;
                       else
                         return ( exp( temp ) );
    default         :  fprintf ( stderr, "ERROR: Illegal Unit Type in ");
                       fprintf ( stderr, "activation function (%d)\n", 
                                 unitType );
                       exit( 1 );
  }
}


/*	ACTIVATION_PRIME -  Compute the activation prime for the unit type and
	activation specified.  Note that no offset is added into the value
	because this confuses the correlation machinery.
*/

float activation_prime  ( int unitType, float value, float sum )
{
  switch  ( unitType )  {
    case SIGMOID     :  return  ( 0.25 - value * value );
    case ASIGMOID    :  return  ( value * ( 1.0 - value ) );
    case VARSIGMOID  :  return  ( ( value - parm.cand.sigMin ) *
                                  ( 1.0 - ( value - parm.cand.sigMin ) / 
                                  ( parm.cand.sigMax - parm.cand.sigMin ) ));
    case GAUSSIAN    :  return  ( sum * (-value) );
    default          :  fprintf ( stderr, "ERROR: Illegal Unit Type in ");
                        fprintf ( stderr, "activation prime (%d)\n",
                                  unitType );
                        exit( 1 );
    }
}


/*	OUTPUT_FUNCTION -  This is essentially the same function as the
	other activation function, except that it uses the out.sigMax and
	out.sigMin instead of the candidate values.  Also, there is a LINEAR
	unit for outputs.  This is often used for continuous floating point
	outputs.
*/

float output_function  ( int unitType, float sum )
{
  switch  ( unitType )  {
    case SIGMOID     :  if  ( sum < -15.0 )
                          return -0.5;
                        if  ( sum > 15.0 )
                          return 0.5;
                        return  ( 1.0 / ( 1.0 + exp( -sum ) ) - 0.5 );
    case LINEAR      :  return( sum );
    case ASIGMOID    :  if  ( sum < -15.0 )
                          return 0.0;
                        if  ( sum > 15.0 )
                          return 1.0;
                        return  ( 1.0 / ( 1.0 + exp( -sum ) ) );
    case VARSIGMOID  :  if  ( sum < -15.0 )
                          return parm.out.sigMin;
                        if  ( sum > 15.0 )
                          return parm.out.sigMax;
                        return  ( (parm.out.sigMax - parm.out.sigMin) / 
                                  (1.0 + exp( -sum ))
                                  + parm.out.sigMin );
    default          :  fprintf ( stderr, "ERROR: Illegal Unit Type in ");
                        fprintf ( stderr, "output function (%d)\n",
                                  unitType );
                        exit( 1 );
  }
}


/*	OUTPUT_PRIME -  Compute the activation prime of an output, this time
	adding in the offset term, to eliminate flat spot.  This can
	dramatically speed up training.
*/

float output_prime  ( int unitType, float value )
{
  switch  ( unitType )  {
    case SIGMOID     :  return ( parm.sigPrimeOffset + 0.25 - value * value );
    case LINEAR      :  return 1.0;
    case ASIGMOID    :  return ( parm.sigPrimeOffset + value * (1.0 - value) );
    case VARSIGMOID  :  return ( parm.sigPrimeOffset +
                               (value - parm.out.sigMin) * 
                               (1.0 - (value - parm.out.sigMin)
                               / (parm.out.sigMax - parm.out.sigMin) ) );
    default          :  fprintf ( stderr, "ERROR: Illegal Unit Type in ");
                        fprintf ( stderr, "output prime (%d)\n", unitType );
                        exit( 1 );
  }
}


/*	STD_DEV -  Compute the standard deviation of a data set.  This is used
	later to compute error index.
*/

float std_dev  ( data_set train, int Npoints, int Nvals )
{
  float cur,		/*  Current output value  */
        sum,		/*  Sum of output values  */
        sumSq;		/*  Sum of the squared output values  */
  int   offsetLeft,	/*  Offset counter  */
        i, j;		/*  Indexing variables  */

  sum 		= 0.0;
  sumSq 	= 0.0;
  offsetLeft 	= netConfig.offset;

  for  ( i = 0 ; i < Npoints ; i++ )
    if  ( train [i].inputs != NULL )  {
      if  ( offsetLeft == 0 )  {
        for  ( j = 0 ; j < Noutputs ; j++ )  {	/*  Compute values for this  */
          cur   =  train [i].outputs [j];	/* output		     */
          sum   += cur;
          sumSq += cur * cur;
        } 
      }  else
        offsetLeft--;
    }  else
      offsetLeft = netConfig.offset;

  /*  Return the standard deviation of this data set  */

  return  ( sqrt( (Nvals * sumSq - sum * sum) / (Nvals * (Nvals - 1.0)) ) );
}

