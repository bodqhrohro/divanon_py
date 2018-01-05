/*	Cascade Correlation Learning Algorithm

	Unix Interface Code
	v1.1
	Matt White  (mwhite+@cmu.edu)
	5/31/94

	QUESTIONS/COMMENTS: neural-bench@cs.cmu.edu

	This code has been placed in public domain by its author.  As a
	matter of common courtesy, anyone using or adapting this code is
	expected to acknowledge the source.  The author would like to hear of
	any attempts to use this system, successful or not.

	This code is currently being maintained by the site contact listed
	above.  If you find a bug, or have a suggestion that will increase the
	usability of this code, please contact the person listed above so that
	the distribution source may be modified accordingly.

	This module is the interface code for a standard unix TTY.  Since
	MS-DOS is a unix derivitive, this code should port fairly easily to
	that platform as well.
*/


/*	Include Files	*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#include "tools.h"
#include "cascor.h"
#include "interface.h"


/*	Data Enumerations	*/

typedef enum  {		/*  Data type.  This is used when reading in	*/
  INT, 			/* parameters to determine how they should be	*/
  FLOAT, 		/* interpreted.					*/
  BOOLEAN,
  UNIT_TYPE, 
  ERR_TYPE
} var_t; 


/*	Data Structure Definitions	*/

/*	PARM_T -  Parameter data structure.  This structure holds the vital
	information necessary to manipulate a single parameter.  In order to
	manipulate all parameters, it will be necessary to declare an array
	of the appropriate size.
*/

typedef struct  {
  char    *name,		/*  This is the search name to use for this  */
				/* parameter.				     */
	  *displayName;		/*  This is the text string to display when  */
				/* a request is made to see the name of this */
				/* parameter.				     */
  var_t   type;			/*  Type of data.  This is used when         */
				/* converting character strings read in as   */
				/* well as when outputting the stored info   */
  void    *parameter;		/*  This is a void pointer to the parameter  */
				/* associated with this entry.		     */
  boolean mod;			/*  If this is true, then the parameter is   */
				/* modifiable after a run starts.  Careful   */
} parm_t;


/*	Constant Declarations	*/

#define NPARMS		34
#define NOT_FOUND	-1


/*	External Global Variables	*/

extern net_data		net;		/*  Internal activation and weights  */
extern net_info		netConfig;	/*  Information of network topology  */
extern output_data	out;		/*  Information about net outputs    */
extern cand_data	cand;		/*  Information about candidate pool */
extern parm_data	parm;		/*  Program run parameters	     */
extern error_data	error;		/*  Network error calculations 	     */
extern alt_data_t	val,		/*  Validation data set		     */
			test;		/*  Test data set		     */
extern run_res_t	runResults;	/*  Totalled results from all trials */

extern int		Ncand,		/*  Number of candidates	    */
			Nunits,		/*  Number of units		    */
			Ninputs,	/*  Number of inputs		    */
			Noutputs,	/*  Number of outputs		    */
			NtrainPts,	/*  Number of training points	    */
			NtrainOutVals,	/*  Number of output values	    */
			epoch;		/*  Epoch just calculated	    */
extern unsigned long	connx;		/*  Number of connection crossings  */
extern boolean		isSeq,			/*  Sequence data set?	    */
			dataLoaded,		/*  Has data been loaded?   */
			interact,		/*  Interact with user?	    */
			helpAvail,		/*  Is help available?	    */
			interruptPending;	/*  User interrupt?	    */


/*	Parameter Table
	~~~~~~~~~~~~~~~
	This data structure is crutial to being able to parse configuration
	files as well as being able to read in parameters from the CLI.
	Since both algorithms use a binary search find the keys to their
	parameters, it is important that all parameters entered into this
	structure are sorted by name, in ascending order.  Equally important
	is that the constant NPARMS be set correctly.  Otherwise, you will
	recieve a compile error or simply not be able to access the last few
	parameters in the table.  NPARMS should be set to the total number of
	parameter entries in the table.
*/

parm_t  parmTable [NPARMS] = {
  { "BIAS", "bias", FLOAT, (void *)&(parm.bias), TRUE },
  { "CANDCHANGETHRESHOLD", 	"candChangeThreshold", FLOAT, 
    (void *)&(parm.cand.changeThresh), TRUE },
  { "CANDDECAY", "candDecay", FLOAT, (void *)&(parm.cand.decay), TRUE },
  { "CANDEPOCHS", "candEpochs", INT, (void *)&(parm.cand.epochs), TRUE },
  { "CANDEPSILON", "candEpsilon", FLOAT, (void *)&(parm.cand.epsilon), TRUE },
  { "CANDMU", "candMu", FLOAT, (void *)&(parm.cand.mu), TRUE },
  { "CANDNEWTYPE", "candNewType", UNIT_TYPE, (void *)&(parm.candNewType), 
    TRUE },
  { "CANDPATIENCE", "candPatience", INT, (void *)&(parm.cand.patience), TRUE },
  { "ERRORINDEXTHRESHOLD", "errorIndexThreshold", FLOAT,
    (void *)&(error.indexThresh), TRUE },
  { "ERRORMEASURE", "errorMeasure", ERR_TYPE, (void *)&(error.measure), TRUE },
  { "MAXNEWUNITS", "maxNewUnits", INT, (void *)&(parm.maxNewUnits), FALSE },
  { "NCAND", "Ncand", INT, (void *)&Ncand, FALSE },
  { "NTRIALS", "Ntrials", INT, (void *)&(parm.Ntrials), TRUE },
  { "OUTDECAY", "outDecay", FLOAT, (void *)&(parm.out.decay), TRUE },
  { "OUTEPOCHS", "outEpochs", INT, (void *)&(parm.out.epochs), TRUE },
  { "OUTEPSILON", "outEpsilon", FLOAT, (void *)&(parm.out.epsilon), TRUE },
  { "OUTERRORTHRESHOLD", "outErrorThreshold", FLOAT, 
    (void *)&(parm.out.changeThresh), TRUE },
  { "OUTMU", "outMu", FLOAT, (void *)&(parm.out.mu), TRUE },
  { "OUTPATIENCE", "outPatience", INT, (void *)&(parm.out.patience), TRUE },
  { "OUTSIGMAX", "outSigMax", FLOAT, (void *)&(parm.out.sigMax), TRUE },
  { "OUTSIGMIN", "outSigMin", FLOAT, (void *)&(parm.out.sigMin), TRUE },
  { "PARSEINBINARY", "parseInBinary", BOOLEAN, (void *)&(parm.parseInBinary),
    FALSE },
  { "PARSEOUTBINARY", "parseOutBinary", BOOLEAN, 
    (void *)&(parm.parseOutBinary), FALSE },
  { "SCORETHRESHOLD", "scoreThreshold", FLOAT, (void *)&(error.scoreThresh),
    TRUE },
  { "SIGMAX", "sigMax", FLOAT, (void *)&(parm.cand.sigMax), TRUE },
  { "SIGMIN", "sigMin", FLOAT, (void *)&(parm.cand.sigMin), TRUE },
  { "SIGPRIMEOFFSET", "sigPrimeOffset", FLOAT, 
     (void *)&(parm.sigPrimeOffset), TRUE },
  { "TEST", "test", BOOLEAN, (void *)&(parm.test), FALSE }, 
  { "USECACHE", "useCache", BOOLEAN, (void *)&(parm.useCache), FALSE },
  { "VALIDATE", "validate", BOOLEAN, (void *)&(parm.validate), FALSE },
  { "VALPATIENCE", "valPatience", INT, (void *)&(parm.valPatience), TRUE },
  { "WEIGHTMULTIPLIER", "weightMultiplier", FLOAT, 
     (void *)&(parm.weightMult), TRUE },
  { "WEIGHTRANGE", "weightRange", FLOAT, (void *)&(parm.weightRange), TRUE },
  { "WINRADIUS", "winRadius", INT, (void *)&(parm.winRadius), FALSE }
};


/*	Function Prototypes	*/

boolean	spec_parm	( boolean, char *, char *, char * );
void	get_data	( char * );
void	get_config	( boolean, char * );
void	get_weights	( char * );
void	save_config	( char * );
void	sel_wfile	( char * );
void	change_out	( char *, char * );
void	list_keys	( boolean );

void	change_parm	( boolean, char *, char * );
void	get_val		( int, char * );
void	set_parm	( int, char * );

void	load_data	( char *, unit_type ** );

void	parse_config	( boolean, char * );
void	process_line	( boolean, char * );
int	find_key	( char * );

char	*ttoa		( unit_type );
char	*etoa		( error_t );
char	*ptoa		( int );
char	*stoa		( status_t );

error_t		atoe		( char * );
unit_type	atot		( char * );


/************************* Output Functions **********************************/

/*	PROG_ID -  Identify the program to the user.  Give some basic
	information about this compiled version.
*/

void  prog_id	( void )
{
  printf  ( "Cascade Correlation Algorithm  v%s (%s)\n", VERSION, 
            RELEASE_DATE );
  printf  ("  compiled: %s  %s\n", __DATE__, __TIME__ );
#ifdef CONNX
  printf  ("  connection crossing statistics ENABLED\n");
#else
  printf  ("  connection crossing statistics DISABLED\n");
#endif
  printf  ("  direct questions to: %s\n\n\n", CONTACT );
} 


/*	LIST_PARMS -  List out calculated parameters.  This is pretty much the
	complete picture.  If no data set has been loaded, this function will
	seg fault.
*/

void  list_parms  ( void )
{
  int i;	/*  Indexing variable  */

  /*  Print run parameters  */

  printf  ("Run Parameters\n");
  printf  ("  Trials: %d\t\tCache: %s\n", parm.Ntrials, 
           btoa( parm.useCache, 3 ) );
  printf  ("  Max new units: %d\tWindow radius: %d\tBias: %6.3f\n",
           parm.maxNewUnits, parm.winRadius, parm.bias );
  printf  ("  Sigmoid prime offset: %6.3f\tWeight range: +/-%5.3f\n",
           parm.sigPrimeOffset, parm.weightRange );
  printf  ("  Weight saves: %s\tFilename: %s\n\n", btoa( parm.saveWeights, 3 ),
           parm.weightFile );

  /*  Print information on the data set  */

  printf  ("Training Set Information\n");
  printf  ("  Data file: %s\tProtocol: %s\n", parm.dataFile, 
           ptoa( netConfig.protocol ) );
  printf  ("  Validation: %s\tValidation patience: %d\tTest: %s\n",
           btoa( parm.validate, 3 ), parm.valPatience, btoa( parm.test, 3 ) );
  printf  ("  Training points: %d\tValidation points: %d\tTest points: %d\n",
           netConfig.train_pts - netConfig.train_seg, val.Npts, test.Npts );
  printf  ("  Inputs: %d\tOutputs: %d\n\n", netConfig.inputs, Noutputs );

  /*  Print information on the outputs  */

  i = 0;
  printf  ("Output Types\n  ");
  while  ( i < Noutputs )  {
    i++;
    printf  ("%2d: %10s     ", i, ttoa( out.types [i-1] ) );
    if  ( (i % 4) == 0 )
      printf  ("\n  ");
  }
  if  ( (i%4) != 0 )
    printf ("\n");
  printf ("\n");

  /*  Print information on how error is calculated  */

  printf  ("Error Parameters\n");
  printf  ("  Error index threshold: %6.3f\t\tScore threshold: %6.3f\n",
           error.indexThresh, error.scoreThresh );
  printf  ("  Error measure: %s\n\n", etoa( error.measure ) );

  /*  Print information on how outputs will be trained  */

  printf  ("Output Unit Parameters\n");
  printf  ("  Epochs: %4d\tChange threshold: %5.3f\t\tPatience: %3d\n",
           parm.out.epochs, parm.out.changeThresh, parm.out.patience );
  printf  ("  Epsilon:  %5.3f\tDecay: %6.4f\t\tMu: %5.3f\n",
           parm.out.epsilon, parm.out.decay, parm.out.mu );
  printf  ("  Sigmoid max: %5.3f\tSigmoid min: %5.3f\n\n",
           parm.out.sigMax, parm.out.sigMin );

  /*  Print information on how candidates will be trained  */

  printf  ("Candidate Unit Parameters\n");
  printf  ("  Number: %3d\tNew unit type: %s\n", 
           Ncand, ttoa( parm.candNewType ) );
  printf  ("  Epochs: %4d\tChange threshold: %5.3f\t\tPatience: %3d\n",
            parm.cand.epochs, parm.cand.changeThresh, parm.cand.patience );
  printf  ("  Epsilon: %5.3f\tDecay: %6.4f\t\tMu: %5.3f\n",
           parm.cand.epsilon, parm.cand.decay, parm.cand.mu );
  printf  ("  Sigmoid max: %5.3f\tSigmoid min: %5.3f\tWeight mult: %5.3f\n\n",
           parm.cand.sigMax, parm.cand.sigMin, parm.weightMult ); 
}


/*	OUTPUT_BEGIN_TRIAL -  Inform the user that a new trial is starting.
	Record the time that this takes place.
*/

void  output_begin_trial  ( int trialNum, time_t *startTime )
{
  time ( startTime );
  printf  ("\n\nTrial %d begun at %s\n", trialNum, ctime( startTime ));
} 


/*	OUTPUT_TRAIN_RESULTS -  Inform the user as to how training the outputs
	went.
*/

void  output_train_results  ( status_t status )
{
  int i = 0, j;		/*  Indexing variables	*/

  /*  Print general statistics	*/

  printf  ("\n  End Output Training Cycle (%s)\n", stoa( status ) );
  printf  ("    Epoch: %d", epoch );
#ifdef CONNX
  printf  ("\t\tConnection crossings: %d\n", connx );
#else
  printf  ("\n");
#endif
  if  ( error.measure == BITS )
    printf  ("    Error bits: %d\t", error.bits );
  else
    printf  ("    Error index: %6.3f\t", error.index );
  printf  ("True error: %8.3f\tSum squared error: %8.3f\n", error.trueErr,
           error.sumSqErr );
  
  /*  Print the weight values for the outputs	*/

  for  ( i = 0 ; i < Noutputs ; i++ )  {
    printf  ("    Output %2d:  ",i+1);
    j = 0;
    while  ( j < Nunits )  {
      printf  ("%8.3f  ", out.weights [i][j] );
      j++;
      if  ( j == Nunits )
        printf  ("\n");
      else if  ( (j % 6) == 0 )
        printf  ("\n                ");
    }
  }
  printf ("\n");
}


/*	OUTPUT_VAL_RESULTS -  Print the results of the validation epoch.
*/

void  output_val_results  ( error_data err )
{
  printf  ("  Validation Epoch\n");
  if  ( error.measure == BITS )
    printf  ("    Error bits: %d\t", err.bits);
  else
    printf  ("    Error index: %8.3\t", err.index );
  printf  ("True error: %8.3f\tSum squared error: %8.3f\n", err.trueErr,
           err.sumSqErr );
  printf  ("    Best true error: %8.3f\tPasses until stagnation: %d\n\n",
           val.bestScore, parm.valPatience - ( Nunits - val.bestPass ) );
}


/*	OUTPUT_CAND_RESULTS -  Output the results of the candidate training
	phase.
*/

void  output_cand_results  ( status_t status )
{
  int i = 0;	/*  Indexing variable	*/

  /*  Print general information  */

  printf  ("  End Candidate Training Cycle (%s)\n", stoa( status ) );
  printf  ("    Epoch: %d", epoch );
#ifdef CONNX
  printf  ("\t\tConnection crossings: %d\n", connx );
#else
  printf  ("\n");
#endif
  printf  ("    Adding unit: %d\tUnit type: %s\tCorrelation: %8.3f\n",
           cand.best + 1, ttoa( cand.types [cand.best] ), cand.bestScore );

  /*  Print the weights for the new unit	*/

  printf  ("    Unit %2d:  ", Nunits);
 
  while  ( i < (Nunits-1) )  {
    printf  ("%8.3f  ", net.weights [Nunits-1][i] );
    i++;
    if  ( i == (Nunits - 1) )
      printf  ("\n");
    else if  ( ( i % 6 ) == 0 )
      printf  ("\n              ");
  }
  printf ("\n");
}
 

/*	OUTPUT_TRIAL_RESULTS -  Print the final results for this trial.
	Compute trial run time and output that.  Store vital information
	in the accumulater runResults, so that run results can later be
	calculated.
*/

void  output_trial_results  ( status_t finalStat, int trial, time_t startTime )
{
  time_t      endTime;		/*  Time that this trial ended	*/
  float       runTime,		/*  Time that this trial took	*/
              errIndex,		/*  Error index for the final epoch	*/
              perCorrect;	/*  Percent correct of the final epoch	*/

  /*  Calculate the run time	*/

  time ( &endTime );
  runTime = endTime - startTime;

  /*  Did we win?  */

  if  ( finalStat == WIN )
    runResults.Nvictories++;

  /*  Store statistics in the run results data structure.  Make sure we	 */
  /* calculate the percent correct and error index from the correct data */
  /* set								 */

  runResults.Nepochs      += epoch;
#ifdef CONNX
  runResults.crossingsSec += connx / runTime;
#endif
  runResults.errorBits    += error.bits;
  if  ( parm.test )  {
    perCorrect                 =  ( (float) (test.NoutVals - error.bits) ) /
                                  test.NoutVals * 100.0;
    errIndex                   =  ERROR_INDEX( error.trueErr, test.stdDev,
                                               test.NoutVals );
  }  else  {
    perCorrect                 = ( (float) (NtrainOutVals - error.bits) ) /
                                 NtrainOutVals * 100.0;
    errIndex                   = error.index;
  }
  runResults.percentCorrect  += perCorrect;
  runResults.errorIndex      += errIndex;
  runResults.trueError       += error.trueErr;
  runResults.sumSqError      += error.sumSqErr;
  runResults.Nunits          += Nunits;
  runResults.runTime         += runTime;
    
  /*  Output the results of the trial	*/

  printf ("  End Trial Results\n");
  printf ("    Epochs: %d\tAverage epoch time: %8.3f sec (%.2f epochs/sec)\n"
          , epoch, runTime / epoch, epoch / runTime );
#ifdef CONNX
  printf ("    Connection crossings: %d\tCrossings per second: %10.2f\n",
          connx, connx / runTime );
#endif
  printf ("    Total units: %d\t\t\tHidden units: %d\n", Nunits, 
          Nunits - Ninputs - 1 );

  if  ( parm.test )
    printf ("    Test results:     ");
  else
    printf ("    Training results: ");
  printf  ("True error: %8.3f\tSum squared error: %8.3f\n", error.trueErr,
           error.sumSqErr );
  if  ( error.measure == BITS )
    printf ("                      Error bits: %d\t\tPercent correct: %6.2f\n",
            error.bits, perCorrect );
  else
    printf ("                      Error index: %8.2f\n", errIndex);

  printf ("\n");
  printf ("Trial %d ended at %s\n\n", trial, ctime( &endTime ) );

  /*  Give the user the opportunity to save these parameters, in case this  */
  /* run came out particularly well.					    */

  if  ( interact )  {
    printf  ("Save configuration to file (yN)? ");
    if  ( get_yn( NO ) )
      save_config ( NULL );
  }
}


/*	OUTPUT_RUN_RESULTS -  Average the results of all the trials and output
	the results for this run.
*/

void  output_run_results  ( void )
{
  int Ntrials = parm.Ntrials;	/*  Number of trials run	*/

  printf  ("\n\n");
  printf  ("Run Results\n");
  printf  ("~~~~~~~~~~~\n");
  printf  ("  %d trials\t%d victories\t%d defeats\n",
           Ntrials, runResults.Nvictories, 
           Ntrials - runResults.Nvictories );
  printf  ("  Run time: %d hrs  %d min  %d sec",
           (((int)runResults.runTime) / 3600),
           (((int)runResults.runTime) % 3600) / 60, 
           ((int)runResults.runTime) % 60 );
#ifdef CONNX
  printf  ("\t\t%8.1f conn/sec\n", runResults.crossingsSec / Ntrials );
#else
  printf  ("\n");
#endif
  printf  ("  Ave epochs: %8.1f\t\tAve hidden units: %5.1f\n",
            ((float)runResults.Nepochs) / Ntrials,
            ((float)runResults.Nunits) / Ntrials - Ninputs - 1 );
  printf  ("  Ave true error: %8.3f\tAve sum squared error: %8.3f\n",
           runResults.trueError / Ntrials, runResults.sumSqError / Ntrials );
  if  ( error.measure == BITS )
    printf ("  Ave bits wrong: %8.1f\tAve percent correct: %8.1f\n",
            ((float)runResults.errorBits) / Ntrials, 
            runResults.percentCorrect / Ntrials );
  else
    printf ("  Ave error index: %8.1f\n", runResults.errorIndex / Ntrials );
  printf ("\n\n");
} 


/************************** CLI Utilities ************************************/

/*	EXEC_COMMAND_LINE -  This function interprets the arguments passed on
	the command line, and performs appropriate actions based on those.
*/

void exec_command_line  ( int argc, char *argv [] )
{
  /*  Check for too many arguments.  If so, give a template to the user  */

  if  ( argc > 4 )  {
    fprintf  ( stderr, "Usage:  %s [-i] [<data set>] [<configuration file>]\n",
	       argv [0] );
    fprintf  ( stderr, "          -i :  Start in interactive mode\n\n" );
    exit  ( 0 );
  }

  /*  Read in the arguments from the line  */

  if  ( argc > 1 )  {
    /*  Check for startup in interactive mode  */

    interact = !strcmp ( "-i", argv [1] );

    /*  Check for starting configuration file  */

    if  ( argc > (interact + 2) )
      parse_config  ( FALSE, argv [interact + 2] );

    /*  Check for starting data set file  */

    if  ( argc > (interact + 1) )
      load_data  ( argv [interact + 1], &(out.types) );
  }

  /*  If data set has not been loaded from command line, put program into  */
  /* interaction mode.							   */

  if  ( !interact )
    interact = !dataLoaded;
}


/*	CHANGE_PARMS -  This is the core of the cascor CLI.  It reads commands
	from the command line and then modifies parameters accordingly.  It
	also provides file manipulation facilities for the user.
*/

void change_parms  ( boolean inRun )
{
  char    line [81],		/*  This is the line read in             */
	  *parm,		/*  The parameter to be modified         */
	  *parmV,		/*  The new value for that parameter     */
	  *parmV2,		/*  Same for parameters with two values  */
	  *seperators = " \t";	/*  Seperator string for strtok   	 */

  /*  Tell the user that they are in CLI  */

  printf  ("Cascor v%s CLI (Run ", VERSION );
  if  ( inRun )
    printf ("started)\n");
  else
    printf ("NOT started)\n");
  printf  ("Enter command, 'help', or '?'.\n");


  while  ( TRUE )  {
    printf  ("> ");	/*  Print prompt and get value  */
    gets ( line );

    /*  Get parameters from the input line  */

    parm   = strtok  ( line, seperators );
    parmV  = strtok  ( NULL, seperators );
    parmV2 = strtok  ( NULL, seperators );

    if  ( parm == NULL )
      continue;

    /*  Check to see if the user wants to start the run  */

    if  ( !strcmp ( parm, "go" ) )  {
      if  ( !dataLoaded )
	printf  ( "Cannot continue until a data set has been loaded.\n" );
      else  {
        list_parms  ( );
        printf  ("\nUse these parameters (Yn)? ");
        if  ( get_yn( YES ) )
	  break;
      }
      continue;
    }

    /*  Check to see if it is time to stop the program  */

    if  ( !strcmp ( parm, "quit" ) )  {
      printf  ("Really quit (yN)? ");
      if  ( get_yn( NO ) )
	exit( 0 );
      else
        continue;
    }

    /*  Check to see if this is a special file function, otherwise modify  */
    /* the parameter table						   */

    if  ( !spec_parm ( inRun, parm, parmV, parmV2 ) )
      change_parm ( inRun, parm, parmV );
  }
}    


/*	SPEC_PARM -  This function handles a group of special case parameters
	that require action be taken rather a parameter modified.
*/

boolean  spec_parm  ( boolean inRun, char *parm, char *parmV, char *parmV2 )
{
  /*  These are the keys for the special parameters, to figure out what  */
  /* the user is trying to do.						 */

  char *specParm [] = { "LOADDATA", "LOADCONFIG",
			"SAVECONFIG", "SAVEWEIGHTS", "OUTTYPE", 
			"LIST", "?", "HELP", "INTERACT", "" };
  int  parmNum;		/*  Which action did the user want to take?  */

  /*  Find the parameter among the keys listed above, or not.  */

  str_upper ( parm );
  for  ( parmNum = 0 ; parmNum < 10 ; parmNum++ )
    if  ( !strcmp( specParm [parmNum], parm ) )
      break;

  /*  Determine which action to take.  If the key was not found, return a  */
  /* FALSE value, so that the other parameter changing routine can take a  */
  /* crack at it.							   */

  switch  ( parmNum )  {
    case 0 :	if  ( inRun )
                  printf  ( "Cannot load data set during run.\n" );
                else
                  get_data ( parmV );
		break;
    case 1 :    get_config ( inRun, parmV );
		break;
    case 2 :	save_config ( parmV );
		break;
    case 3 :	sel_wfile ( parmV );
		break;
    case 4 :	if  ( inRun )
                  printf  ( "Cannot change outputs during run.\n" );
                else
                  change_out ( parmV, parmV2 );
		break;
    case 5 :	if  ( dataLoaded )
		  list_parms  ( );
		else  {
		  printf  ("Data set must be loaded before parameters can be");
		  printf  (" listed.\n");
		}
		break;
    case 6 :	list_keys  ( inRun );
		break;
    case 7 :	printf ("\n");
		help  ( parmV, 0, TRUE );
		printf ("\n");
		break;
    case 8 :	interact = !interact;
		printf  ("Interaction mode is now %s\n", btoa( interact, 3 ) );
		break;
    default :	return FALSE;
  }

  return TRUE;	/*  We found the key  */
}		


/*	GET_DATA -  CLI interface to the LOAD_DATA function.  If no file name
	is provided, then prompt for one.  If a data has already been loaded,
	prompt to make sure that the user wants to get rid of it and load some
	new data.
*/

void  get_data  ( char *filename )
{
  char datafile [MAX_PATH + 1];	/*  File name of the data file  */

  if  ( dataLoaded )  {
    printf  ( "Data set already loaded, discard (yN)? " );
    if  ( get_yn ( NO ) )  {
      discard_net  ( &netConfig );
      free( parm.dataFile );
      free( out.types );
    }
    else
      return;
  }
  if  ( filename == NULL )  {
    printf  ( "Filename: " );
    gets( datafile );
  }  else
    strcpy ( datafile, filename );
  load_data  ( datafile, &(out.types) );
}
	

/*	GET_CONFIG -  This function is the CLI interface to the parse_config
	function.  It tells that function whether a run has been started and
	what file to parse.  If no file is provided, the user is prompted for
	one before the program continue.
*/

void  get_config  ( boolean inRun, char *filename )
{
  char datafile [MAX_PATH+1];	/*  Name of file to load  */

  if  ( filename == NULL )  {
    printf  ( "Filename: " );
    gets( datafile );
  }  else
    strcpy ( datafile, filename );
  parse_config  ( inRun, datafile );
}


/*	SAVE_CONFIG -  This function goes through and saves a copy of all the
	modifiable parameters.  If no filename is provided, the program prompts
	the user for one.  If the file name provided has no extension, one is
	added.  If this would cause a file to be overwritten, the user is 
	prompted if he or she wants to proceed.
*/

void  save_config  ( char *filename )
{
  char temp [MAX_PATH + 1],	/*  Temporary file name  */
       datafile [MAX_PATH + 1],	/*  The modified file name  */
       *ext [] = { DATA_EXT, CONFIG_EXT, WEIGHT_EXT };	/*  The extensions  */
  FILE *fptr;			/*  File to save the info under  */
  int  i;			/*  Indexing variable  */

  /*  Prompt for file name  */

  if  ( filename == NULL )  {
    printf  ( "Filename: " );
    gets( temp );
  }  else
    strcpy( temp, filename );

  add_ext  ( temp, datafile, CONFIG_EXT, ext, N_EXT );

  /*  Prompt for over-write  */

  if  ( file_exist( datafile ) )  {
    printf  ( "File (%s) exists, overwrite (yN)? ", datafile );
    if  ( !get_yn( NO ) )
      return;
  }

  /*  Open the save file  */

  if  ( ( fptr = fopen ( datafile, "wt" ) ) == NULL )  {
    fprintf  ( stderr, "\n***ERROR:  Unable to open configuration file (%s)\n",
	       datafile );
    return;
  }

  /*  Loop through the entire parameter table, and save each parameter  */

  fprintf  ( fptr, "#  Cascade Correlation  v%s Configuration File\n", 
             VERSION );

  for  ( i = 0 ; i < NPARMS ; i++ )  {
    fprintf  ( fptr, "%s\t", parmTable [i].displayName );
    switch  ( parmTable [i].type )  {
      case INT:		fprintf ( fptr, "%d\n", 
				  *(int *)parmTable [i].parameter );
			break;
      case FLOAT:	fprintf ( fptr, "%f\n",
				  *(float *)parmTable [i].parameter );
			break;
      case BOOLEAN:	fprintf ( fptr, "%s\n",
				  btoa(*(boolean *)parmTable [i].parameter,1));
			break;
      case UNIT_TYPE:	fprintf ( fptr, "%s\n",
			          ttoa(*(unit_type *)parmTable [i].parameter));
			break;
      case ERR_TYPE:	fprintf ( fptr, "%s\n",
				  etoa( *(error_t *)parmTable [i].parameter ));
    }
  }

  fclose ( fptr );	/*  Close the data file  */
}


/*	SEL_WFILE -  Select a file to save weights to.  If weights are already
	being saved, ask if the user wants to turn them off.  Otherwise get the
	base filename to save weights under.  If the file name provided has an
	extension, that extension is removed.
*/

void  sel_wfile  ( char *filename )
{
  char temp [MAX_PATH + 1],	/*  Temporary file name  */
       datafile [MAX_PATH + 1],	/*  Name of weight file  */
       *ext [] = { DATA_EXT, CONFIG_EXT, WEIGHT_EXT }; /*  Extension list  */

  /*  Prompt to see if the weight saves should be turned off or changed  */

  if  ( parm.saveWeights )  {
    printf  ("Weight saves are on, turn off (yN)? ");
    if  ( get_yn( NO ) )  {
      free ( parm.weightFile );
      parm.saveWeights = FALSE;
      return;
    }
    printf  ("Weights currently being saved to file: %s\n", parm.weightFile );
    printf  ("Change save file (yN)? ");
    if  ( !get_yn( NO ) )
      return;
  }

  /*  Prompt for file name  */

  if  ( filename == NULL )  {
    printf  ( "Filename: " );
    gets( temp );
  }  else
    strcpy( temp, filename );

  /*  Remove extension and then put file name into the parameter structure  */

  add_ext  ( temp, datafile, "", ext, N_EXT );
  
  parm.weightFile = (char *)alloc_mem ( strlen( datafile ) + 1, sizeof( char ),
					"Select Weight File" );
  strcpy ( parm.weightFile, datafile );
  parm.saveWeights = TRUE;
}


/*	CHANGE_OUT -  This function allows the user to change parameters from
	there heuristicly chosen defaults to something else that the user
	wants.  As is usual for these functions, anything not provided is
	prompted for.
*/

void  change_out  ( char *outNum, char *unitType )
{
  int    	outN;		/*  Number of output to change  */
  char   	temp [21];	/*  Character string read in    */
  unit_type	type;		/*  New type for the unit       */

  /*  Check to see that data has been loaded  */

  if  ( !dataLoaded )  {
    printf  ("Data must be loaded before outputs can be changed\n");
    return;
  }

  /*  Figure out which output to change  */

  if  ( outNum == NULL )  {
    outNum = temp;
    printf ("Output number: ");
    gets ( outNum );
  }
  outN = atoi( outNum );
  if  ( outN >= Noutputs )  {
    printf  ("There aren't that many outputs in the network!\n");
    return;
  }
  
  /*  Get new type for unit and then change the setting  */

  if  ( unitType == NULL )  {
    unitType = temp;
    printf ("Unit type: ");
    gets ( unitType );
  }
  type = atot( unitType );
  if  ( type == UNINITIALIZED )  {
    printf  ("Invalid output type.\n");
    return;
  }
  out.types [outN] = type;
}


/*	LIST_KEYS -  This function loops through the available keys and prints
	their current values.  If a key cannot be changed, it is not listed.
	Also listed are the special keys, except that they have no values.
*/

void  list_keys  ( boolean inRun )
{
  int i;	/*  Indexing variables  */

  printf  ("The following keys are currently available:\n");

  for  ( i = 0 ; i < NPARMS ; i++ )  {
    if  ( inRun && !parmTable [i].mod )
      continue;
    printf  ( "  %s ", parmTable [i].displayName );
    switch  ( parmTable [i].type )  {
      case INT:		printf ( "[%d]\n", 
				 *(int *)parmTable [i].parameter );
			break;
      case FLOAT:	printf ( "[%f]\n",
				 *(float *)parmTable [i].parameter );
			break;
      case BOOLEAN:	printf ( "[%s]\n",
				 btoa(*(boolean *)parmTable [i].parameter,1) );
			break;
      case UNIT_TYPE:	printf ( "[%s]\n",
				 ttoa(*(unit_type *)parmTable [i].parameter));
			break;
      case ERR_TYPE:	printf ( "[%s]\n",
				 etoa( *(error_t *)parmTable [i].parameter ) );
    }
  }
  printf ("\nLegal keys also include: loadData, loadConfig,\n");
  printf ("                         saveConfig, saveWeights, outType,\n");
  printf ("                         interact, list, help, ?, go, quit\n");
}


/*	CHANGE_PARM -  This function is used to change regular parameters.
	If a run has been started, some parameters are not modifiable.
*/

void  change_parm  ( boolean inRun, char *parm, char *parmV )
{
  int  loc;		/*  Key's location in the parm table  */
  char val [41];	/*  The new value of the parameter chosen  */

  /*  Locate the key in the parameter table  */

  loc  = find_key  ( parm );
  if  ( loc == NOT_FOUND )  {
    printf  ("Key (%s) not found.\n", parm );
    return;
  }

  /*  Check to see that the parameter is modifiable  */

  if  ( inRun && !parmTable [loc].mod )  {
    printf  ( "Parameter %s is not modifiable during program run.\n",
              parmTable [loc].displayName );
    return;
  }

  /*  Get the value for this parameter  */

  if  ( parmV != NULL )		
    strcpy ( val, parmV );
  else
    get_val ( loc, val );

  set_parm ( loc, val );	/*  Set the parameter  */
}


/*	GET_VAL -  Prompt the user for a value of the indicated parameter.
	Give information about the type of parameter and it's current value.
	If help on this parameter is available, display that as well.
*/

void  get_val  ( int loc, char *value )
{
  printf  ("\nParameter:\t%s\n", parmTable [loc].displayName );
  switch  ( parmTable [loc].type )  {
    case INT 		: printf ( "Type:\t\tInteger\n" );
			  printf ( "Value:\t\t%d", 
                                   *(int *)parmTable [loc].parameter );
			  break;
    case FLOAT		: printf ( "Type:\t\tFloating Point\n" );
			  printf ( "Value:\t\t%f",
				   *(float *)parmTable [loc].parameter );
			  break;
    case BOOLEAN	: printf ( "Type:\t\tBoolean\n" );
			  printf ( "Value:\t\t%s",
				  btoa( *(boolean *)parmTable [loc].parameter,
				  1 ) );
			  break;
    case UNIT_TYPE	: printf ( "Type:\t\tUnit Type\n" );
			  printf ( "Value:\t\t%s",
				 ttoa(*(unit_type *)parmTable[loc].parameter));
			  break;
    case ERR_TYPE	: printf ( "Type:\t\tError Type\n" );
			  printf ( "Value:\t\t%s",
				  etoa(*(error_t *)parmTable [loc].parameter));
			  break;
  }

  /*  Print help  */
 
  if  ( helpAvail )  {
    printf ("\n\n");
    help ( parmTable [loc].name, 2, FALSE );
  }

  /*  Get new value  */

  printf ("\nNew value: ");
  gets ( value );
}


/*	SET_PARM -  Set the specified parameter.  Type conversions from
	character strings are handled by this function.
*/

void  set_parm  ( int loc, char *value )
{
  switch  ( parmTable [loc].type )  {
    case INT:		*(int *)parmTable [loc].parameter = atoi( value );
			break;
    case FLOAT:		*(float *)parmTable [loc].parameter = atof( value );
			break;
    case BOOLEAN:	*(boolean *)parmTable [loc].parameter = atob( value );
			break;
    case UNIT_TYPE:	*(unit_type *)parmTable[loc].parameter = atot( value );
			break;
    case ERR_TYPE:	*(error_t *)parmTable [loc].parameter = atoe( value );
			break;
  }
}


/*	LOAD_DATA -  Load a new data set.  Figure out the values of associated
	global values.
*/

void load_data  ( char *filename, unit_type **outTypes )
{
  int      enumType,	/*  How to deal with enumerations  */
           i;		/*  Indexing variable  */
  char     *fn			= "Load Data";	/*  Function name  */
  boolean  parseOK;	/*  Did parsing go ok?  */

  /*  Figure out how to deal with enumerations and then parse the data file  */

  enumType =  ( parm.parseInBinary ? BINARY_IN : UNARY_IN ) |
              ( parm.parseOutBinary ? BINARY_OUT : UNARY_OUT );

  parseOK = parse ( filename, enumType, BIN_POS, BIN_NEG, &netConfig );

  if  ( parseOK )  {
    isSeq 	= ( netConfig.protocol == 2 );

    Ninputs		=  netConfig.inputs;
    if  ( isSeq )
      Ninputs		+= 2 * (parm.winRadius) * Ninputs;
    Noutputs		=  netConfig.outputs;
    NtrainOutVals	=  (netConfig.train_pts - netConfig.train_seg) *
                           Noutputs;

    parm.dataFile = (char *) alloc_mem( strlen( filename ) + 1,
                                        sizeof( char ), fn );
    strcpy ( parm.dataFile, filename );

    /*  Check for a validation data set, otherwise use the test set.  */

    if  ( netConfig.validate_pts != 0 )  {
      val.Npts  	= netConfig.validate_pts;
      val.Nsegs		= netConfig.validate_seg;
      val.data		= netConfig.validate;
    }  else
    if  ( netConfig.test_pts != 0 )  {
      val.Npts		= netConfig.test_pts;
      val.Nsegs		= netConfig.test_seg;
      val.data		= netConfig.test;
    }  else
      parm.validate	= FALSE;
    val.NoutVals	= (val.Npts - val.Nsegs) * Noutputs;

    /*  Check for the presence of a test set  */

    if  ( netConfig.test_pts != 0 )  {
      test.Npts		= netConfig.test_pts;
      test.Nsegs	= netConfig.test_seg;
      test.NoutVals     = (netConfig.test_pts - netConfig.test_seg) *
                          Noutputs;
      test.data		= netConfig.test;

    }  else
      parm.test	= FALSE;

    error.measure	= BITS;

    /*  Allocate memory for output types.  Assign them a linear function if  */
    /* they are continuous outputs and sigmoidal function if they are binary */
    /* Also, if any linear functions are assigned, switch from a errorBits   */
    /* measure to an error index measure.				     */

    (*outTypes) = (unit_type *)alloc_mem ( Noutputs, sizeof( unit_type ), fn );
    for  ( i = 0 ; i < Noutputs ; i++ )
      if  ( netConfig.out_type [i] == BINARY )
        if  ( (BIN_POS == 0.5) && (BIN_NEG == -0.5) )
          (*outTypes) [i]  = SIGMOID;
        else  if  ( (BIN_POS == 1.0) && (BIN_NEG == 0.0) )
          (*outTypes) [i]  = ASIGMOID;
        else
          (*outTypes) [i]  = VARSIGMOID;
      else  {
        (*outTypes) [i] 	= LINEAR;
	error.measure		= INDEX;
      }

    NtrainPts	= netConfig.train_pts - netConfig.train_seg;
  }
  dataLoaded = parseOK;
}


/*	PARSE_CONFIG -  Goes through and modifies those parameters that are 
	specified in the file named.  If a parameter is not modifiable in the
	current state of the program, then a message is printed out to the
	user and execution continues.
*/

void  parse_config  ( boolean inRun, char *fileName )
{
  FILE *configFile;	/*  File pointer to the configuration file  	*/
  char buffer [256];	/*  Input buffer from the file			*/

  /*  If unable to open the configuration file, print an error message	*/
  /* and return								*/

  if  ( ( configFile = fopen ( fileName, "r" )) == NULL )  {
    fprintf  ( stderr, "***ERROR: Unable to open configuration file: %s\n",
               fileName );
    return;
  }

  /*  Repeatedly read lines and process them  */

  fgets ( buffer, 255, configFile );
  while ( !feof( configFile ) )  {
    process_line ( inRun, buffer );
    fgets( buffer, 255, configFile );
  }

  fclose( configFile );
}


/*	PROCESS_LINE -  Takes the tokens in the buffer passed to it and
	performs the necessary changes in the parameters.  There may be
	more than one parameter on a line.
*/

void  process_line  ( boolean inRun, char *buffer )
{
  int         location;		/*  Location of the key in the parse table  */
  char        *keyTok,		/*  Parameter token to modify		    */
              *valTok;		/*  New value for this parameter	    */
  static char *seperators = " \t\v\f\r\n,";	/*  Seperators for strtok   */

  if  ( *buffer == '#' )	/*  Check for comment cards  */
    return;

  /*  Get first token from the line (parameter)	*/

  keyTok = strtok ( buffer, seperators );

  while  ( keyTok != NULL )  {
    location = find_key ( keyTok );	/*  Find the token in parmTable  */

    if  ( location != NOT_FOUND )  {

      /*  Check that token is modifiable in current program state  */

      if  ( inRun && !parmTable [location].mod )  {
        printf  ( "Parameter %s is not modifiable during program run.\n",
                  parmTable [location].displayName );
      }  else  {

	/*  Get a value for this parameter  */

        if  ( (valTok = strtok ( NULL, seperators )) == NULL )  {
          printf  ( "No value for %s\n", keyTok );
          return;
        }

	/*  Convert this parameter to the appropriate type and store it  */

        switch  ( parmTable [location].type )  {
          case INT        :  *((int *) parmTable [location].parameter) = 
                                                                atoi( valTok );
                             break;
          case FLOAT      :  *((float *) parmTable [location].parameter) =
                                                                atof( valTok );
                             break;
          case BOOLEAN    :  *((boolean *) parmTable [location].parameter) =
                                                                atob( valTok );
                             break;
          case UNIT_TYPE  :  *((unit_type *) parmTable [location].parameter) =
                                                                atot( valTok );
                             break;
          case ERR_TYPE   :  *((error_t *) parmTable [location].parameter) =
                                                                atoe( valTok );
                             break;
        }
      }
    }  else
      printf ( "%s not found, continuing...\n", keyTok );

    /*  Get the next parameter from this line  */
    
    keyTok = strtok( NULL, seperators );
  }
}


/*	FIND_KEY -  Conduct a binary search through the parameter table and
	find the token requested.  If the token is found return its location,
	otherwise return a NOT_FOUND value.
*/

int  find_key  ( char *key )
{
  int location,	/*  Location of the token	*/
      start,	/*  Start of the area being searched	*/
      end,	/*  End of the area being searched	*/
      dir;	/*  Direction to search		*/

  str_upper( key );	/*  Initialize the search	*/
  start     = 0;
  end       = NPARMS;
  location  = end / 2;

  while ( start <= end )  {
    if  ( (dir = strcmp( key, parmTable [location].name )) == 0 )
      return location;
    if  ( dir < 0 )
      end = location - 1;
    else
      start = location + 1; 
    location = (start + end) / 2;
  }

  return NOT_FOUND;
}


/*	SAVE_WEIGHTS -  Save information on the topology of the network so
	that another program can rebuild the net at another date.  The data
	is stored in a file of fileName.  It will be headed with comments on
	what the data file was and which trial this is, as well as what time
	it started.
*/

void  save_weights  ( char *fileName, boolean interact,
		      int trial, time_t startTime )
{
  FILE *weightFile;		/*  Pointer to the data file  	*/
  char *file,			/*  Modified file name		*/
       *fn = "Save Weights";	/*  Function name for alloc_mem */
  int  fileNameLen,		/*  Length of the file name	*/
       i, j;			/*  Indexing variables		*/

  /*  Generate a file name from the base file name given  */

  fileNameLen = strlen( fileName ) + 10;
  file        = (char *) alloc_mem ( fileNameLen, sizeof( char ), fn );
  sprintf ( file, "%s-%d.net", fileName, trial );

  /*  Check for the existance of the file  */

  if  ( file_exist ( file ) )
    if  ( interact )  {
      fprintf  ( stderr, "File %s already exists, overwrite (yN)? ", file );
      if  ( !get_yn ( NO ) )  {
        fprintf  ( stderr, "Turn off weight saves (Yn)? " );
        parm.saveWeights = !get_yn ( YES );
        return;
      }
    }  else
      return;

  /*  Open the file  */

  if  ( ( weightFile = fopen ( file, "w" )) == NULL )  {
    fprintf  ( stderr, "\n***ERROR-  Unable to open weight file: %s\n", file );
    fprintf  ( stderr, "           Continuing...\n\n");
    return;
  }  else
    printf  ("Saving weights to file: %s\n\n", file );

  /*  Print the header  */

  fprintf  ( weightFile, "# %s  trial: %d  %s", parm.dataFile, trial,
             ctime( &startTime ) );
  fprintf  ( weightFile, "Ninputs: %d\tNunits: %d\tNoutputs: %d\n", Ninputs, 
             Nunits, Noutputs );
  fprintf  ( weightFile, "winRad: %d\tbias: %f\n", parm.winRadius, parm.bias );
  fprintf  ( weightFile, "outSigMax: %f\toutSigMin: %f\n",
             parm.out.sigMax, parm.out.sigMin );
  fprintf  ( weightFile, "candSigMax: %f\t\tcandSigMin: %f\n",
             parm.cand.sigMax, parm.cand.sigMin );

  /*  Print the output types  */

  fprintf  ( weightFile, "# Output types\n");
  fprintf  ( weightFile, "$\n" );
  i = 0;
  while  ( i < Noutputs )  {
    fprintf  ( weightFile, "%s  ", ttoa( out.types [i] ) );
    i++;
    if  (  ( i % 6 ) == 0 )
      fprintf ( weightFile, "\n");
  }
  if  ( ( i % 6 ) != 0 )
    fprintf  ( weightFile,"\n");

  /*  Print the inner unit types  */

  fprintf  ( weightFile, "# Hidden unit types\n" );
  fprintf  ( weightFile, "$\n" );
  i = 0;
  while  ( i < (Nunits - Ninputs - 1) )  {
    fprintf  ( weightFile, "%s  ", ttoa( net.unitTypes [i+Ninputs+1] ) );
    i++;
    if  ( ( i % 6 ) == 0 )
      fprintf ( weightFile, "\n" );
  }
  if  ( ( i % 6 ) != 0 )
    fprintf ( weightFile, "\n");
  
  /*  Print the output weights  */

  for  ( i = 0 ; i < Noutputs ; i++ )  {
    fprintf  ( weightFile, "# Output: %d\n", i + 1 );
    fprintf  ( weightFile, "$\n" );
    j = 0;
    while  ( j < Nunits )  {
      fprintf  ( weightFile, "%f  ", out.weights [i][j] );
      j++;
      if  ( ( j % 6 ) == 0 )
        fprintf  ( weightFile, "\n" );
    }
    if  ( ( j % 6 ) != 0 )
      fprintf  ( weightFile, "\n" );
  }

  /*  Print the hidden unit weights  */

  for  ( i = Ninputs + 1; i < Nunits ; i++ )  {
    fprintf  ( weightFile, "# Hidden unit: %d\n", i - Ninputs );
    fprintf  ( weightFile, "$\n" );
    j = 0;
    while  ( j < i )  {
      fprintf  ( weightFile, "%f  ", net.weights [i][j] );
      j++;
      if  ( ( j % 6 ) == 0 )
        fprintf  ( weightFile, "\n" );
    }
    if  ( ( j % 6 ) != 0 )
      fprintf  ( weightFile, "\n" );
  }

  fclose( weightFile );
}


void  load_weights  ( char *filename )
{
  FILE  *weightFile;
  int   sect = 0, begin_out, begin_hidden;

  if  ( (weightFile = fopen ( filename, "rt" )) == NULL )  {
    fprintf ( stderr, "\nERROR:  Unable to open weight file.\n" );
    fprintf ( stderr, "Continuing...\n" );
    return;
  }

  
}


/*	CHECK_INTERRUPT -  Check to see if an interrupt is pending.  If it is,
	allow the user to change some parameters.  Recalculate certain crutial
	values and then reset the interrupt flag for next time.
*/

void  check_interrupt  ( void )
{
  if  ( interruptPending )  {
    printf        ( "\nSimulation suspended at epoch %d.\n", epoch );
    change_parms  ( TRUE );

    out.shrinkFactor	= parm.out.mu / ( 1.0 + parm.out.mu );
    out.scaledEpsilon	= parm.out.epsilon / ( netConfig.train_pts -
                                               netConfig.train_seg );
    cand.shrinkFactor	= parm.cand.mu / ( 1.0 + parm.cand.mu );
  
    interruptPending = FALSE;
    printf  ( "Simulation continuing...\n" );
  }
}


/*	TRAP_CTRL_C -  This function acts as an interrupt trap.  If the user
	causes an exception by pressing C-c, this function is called, setting
	the interrupt flag and repositioning the interrupt handler.
*/

void  trap_ctrl_c  ( int sig )
{
  interruptPending = TRUE;
  signal  ( SIGINT, trap_ctrl_c );
}


/*	TTOA -  This function takes in a unit type and then returns a string
	that is associated with that type.  This can then be outputted to the
	user using any means desired.
*/

char *ttoa  ( unit_type inVal )
{
  switch  ( inVal )  {
    case VARIED		: return "Varied";
    case SIGMOID	: return "Sigmoid";
    case ASIGMOID	: return "Asigmoid";
    case GAUSSIAN	: return "Gaussian";
    case VARSIGMOID	: return "Varsigmoid";
    case LINEAR		: return "Linear";
    default		: return "Uninitialized";
  }
}


/*	ETOA -  This function takes in, as an argument, an error type.  It 
	converts this type to the associated string values and returns a
	pointer to that string.
*/

char *etoa  ( error_t inVal )
{
  switch  ( inVal )  {
    case BITS	: return "Bits";
    case INDEX  : return "Index";
  }
}


/*	PTOA -  This function takes as its argument a protocol type and returns
	a pointer to the string that is associated with that protocol.
*/

char *ptoa  ( int protocol )
{
  static char *protocolStrings [] = { "IO", "Sequence" };

  return ( protocolStrings [protocol-1] );
}


/*	STOA -  Takes as an argument a variable of type status and returns
	a pointer to the string associated with that status.
*/

char *stoa  ( status_t stat )
{
  switch  ( stat )  {
    case TRAINING	: return "Training";
    case TIMEOUT	: return "Time Out";
    case STAGNANT	: return "Stagnant";
    case WIN		: return "Victory";
  }
}


/*	ATOE -  Takes a string token and returns the error type associated with
	that string.
*/

error_t atoe  ( char *token )
{
  str_upper( token );

  if  ( !strcmp( "BITS", token ) )
    return BITS;
  return INDEX;
}


/*	ATOT -  Takes a character string as its argument.  It then returns the
	unit type represented by this character string, or UNITIALIZED if no
	type was found.
*/

unit_type atot  ( char *token )
{
  static char *typeStrings [] = { "VARIED", "UNITIALIZED", "SIGMOID",
				  "ASIGMOID", "GAUSSIAN", "VARSIGMOID",
                                  "LINEAR" };

  str_upper( token );

  if  ( !strcmp( token, "VARIED" ) )
    return VARIED;
  if  ( !strcmp( token, "SIGMOID" ) )
    return SIGMOID;
  if  ( !strcmp( token, "ASIGMOID" ) )
    return ASIGMOID;
  if  ( !strcmp( token, "GAUSSIAN" ) )
    return GAUSSIAN;
  if  ( !strcmp( token, "VARSIGMOID" ) )
    return VARSIGMOID;
  if  ( !strcmp( token, "LINEAR" ) )
    return LINEAR;

  return UNINITIALIZED;
}
