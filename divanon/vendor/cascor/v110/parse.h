/*	CMU Learning Benchmark Parse Library

	v1.0.4
	Matt White  (mwhite+@cmu.edu)
	10/15/93
*/

#ifndef NET_PARSE
#define NET_PARSE

/*  Data Structure Definitions  */

typedef struct  {		/*  This structure holds a set of network  */
  float *inputs;		/* inputs and outputs, stored as floating  */
  float *outputs;		/* point numbers.			   */
} data_point, *data_set;


/*  This structure contains the information to be passed back to the calling */
/* program.								     */

typedef struct  {	
  int protocol,		/*  Data protocol to use  */
      offset,		/*  Number of inputs to read before an output should */
			/* occur			    		     */
      inputs,		/*  Number of inputs in the network	*/
      outputs,		/*  Number of outputs in the network	*/
      *out_type,        /*  One element for each output, it is set to BINARY */
                        /* if the element is binary, CONT if it is a         */
                        /* standard floating point number                    */
      train_pts,        /*  Number of training points  */
      validate_pts,     /*  Number of validation points  */
      test_pts,         /*  Number of test points  */
      train_seg,        /*  These three variables hold the number of segment */
      validate_seg,     /* markers in each data set.                         */
      test_seg;
  data_set train,	/*  These three data sets contain the data needed to */
	   validate,	/* train and test the network 			     */
	   test;
} net_info;


/*  Constant Declarations  */

#define BINARY_IN  1
#define UNARY_IN   0
#define BINARY_OUT 2
#define UNARY_OUT  0

#define BINARY 1
#define CONT   2

#define IO		1
#define SEQUENCE	2


/*  Function Prototypes  */

int   parse		( char *, int, float, float, net_info * );
void  discard_net	( net_info * );

#endif



