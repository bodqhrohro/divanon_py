/*	CMU Learning Benchmark Parse Library

	v1.1
	Matt White  (mwhite+@cmu.edu)
	5/31/94

	This library contains two functions for the parsing of data files in
	the CMU Learning Benchmark format.  Given the name of the data file,
	information on how to deal with enumerations, and a data structure,
	the parse procedure will build the data structure and fill it with
	information on how to build the network.  A function is also provided
	to destroy this data structure.

	Below is a description of the use of this library.  If anything seems
	unclear, please consult the description file for our data format.  If,
	after consulting this file, you still have questions, please contact
	us at:

		neural-bench@cs.cmu.edu


	Compiling the library:

	  Unpack the shar file and place the six resulting
	  files: parse.c, parse.h, queue.c, queue.h, tools.c,
	  tools.h in your source directory.  Parse.c, tools.c
	  and queue.c should be compiled and linked in with your
	  program.  In addition, any module that will call either
	  parse() or discard_net(), or makes reference to the
	  net_info data structure needs to have the line:

	    #include "parse.h"

	  at the beginning of the file, with the other include
	  statements.


	Use of the parse procedure is as follows:

	  parse  ( char *filename, int parameters, float bin_pos, float bin_neg
                   net_info *network );

	    filename    -  A character string containing the name of the file
		           to parse.

	    parameters  -  An integer containing two switches, one for input
			   and the other for output.  Each switch determines
			   whether each token in an enumeration will be
			   assigned a node, or whether the whole of the
			   enumeration will be assigned binary representations.

			   BINARY_IN, :  Assign the enumeration a binary
			   BINARY_OUT    representation.

			   UNARY_IN,  :  Assign each token in the enumeration
			   UNARY_OUT     its own node.

			   Parameters can be combined with the bitwise OR
			   operator.  Unary parameters are the default, if
			   none are specified.

	    bin_pos,    -  The values to give positive negative binary
            bin_neg        values, respectively.  For sigmoid outputs, these
	                   should be 0.5 and -0.5.  

	    network     -  The address of the data structure to store the
			   information in.  The structure must be of type
			   'net_info', and memory must already be allocated
			   for it.

	  
	  discard_net  ( net_info *network );

	    This procedure destroys the internal data structures built inside
	    of 'network' by parse().  Memory is freed and returned to the
	    general memory pool.


	The following information is returned in the 'net_info' data structure:

	  protocol 	-  This is the format of the data file.  A '1'
			   indicates an IO mapping, a '2' is a 'SEQUENCE'.

	  offset	-  The number of inputs to read before an output should
			   be expected.

	  inputs	-  This is the number of input nodes the network
			   should have.

	  outputs	-  This is the number of output nodes the network
			   should have.

	  out_type      -  This is an array with a number of elements equal
	                   to the number of outputs in the network.  An
			   element in this array is set to BINARY if it 
			   corrosponds to an output that is binary (i.e. it is
			   either one value or another) and set to CONT if
			   that node is a standard floating point number.

	  train_pts,    -  These three numbers indicate how many data points
	  validate_pts,    are in each of these data groups.  End of segment
	  test_pts         markers ARE included in this number.

          train_seg,    -  These three numbers indicate how many end of
          validate_seg,    segment markers are in each of these data groups.
          test_seg

	  train,	-  These three pointers point to lists of data points.
	  validate,	   Each of these data points indicates a list of
	  test		   floating point numbers, one for each node.  An
			   end-of-sequence is marked by NULL inputs and
			   outputs.

		Examples:
        	
		net.train[56].inputs[4] indicates the 5th input in the 57th
		training point in the network 'net'.

		net.test[15].outputs[6] indicates the 7th output in the 16th
		testing point in the network 'net'.


	Revision Log
	~~~~~~~~~~~~
	5/31/94		1.1     Added a 'rosetta stone' feature that prints
				the correspondances between enumerated values
				and their inputs into the network.
	2/20/94         1.0.4   Fixed a bug in which if there were continuous
				inputs/outputs following an enumerated input
				or output, would cause that floating point
				value to overwrite the enumeration.
	10/15/93	1.0.3   Fixed a bug that caused parse to dump core if
				a data file is not found.
	9/30/93		1.0.2	Fixed bug that caused data sets specifying
				a domain for continuous values to be
				rejected as unknown node types.  Fixed
				another bug that caused datasets with more
				than one output to be rejected as having too
				few outputs.  Thanks to Iain Strachan for 
				pointing this out.
	9/24/93		1.0	Initial Release
*/


/*	Include Files		*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#include "parse.h"
#include "queue.h"
#include "tools.h"

/*	Constant Definitions	*/

#define TRUE  1			/*  Don't change these constants, they are  */
#define FALSE 0			/* necessary to the functioning of this     */
				/* library				    */
#define NUM_DELIM       5
#define NUM_COMMANDS    11

#define NO_MEMORY \
  { fprintf ( stderr, "\nERROR: Unable to allocate memory\n\n" ); exit( 1 ); }

/*	Parsing Parameters  (modify to suit)	*/

#define MAX_ENUM_LEN	20	/*  The maximum length of an enumeration  */


/*	Data Structure Definitions	*/

/*  This structure is used to convert from a character-based  */
/* external representation of enumerated types to a numeric   */
/* internal representation 				      */

typedef struct {
  char    *name;
  float   *equiv;  
} enum_cvrt;


/*  This structure contains information necessary to the	*/
/* internal parsing of the data file.  				*/

typedef struct {
  int       parse_mode,		/*  Current segment being parsed */
            parameters,		/*  Parameters passed to the parse function */
            offset_left,	/*  Number of inputs left to read before   */
				/* output				   */
            inputs_left,	/*  Number of inputs left that need to be  */
				/* declared as a certain type		   */
            outputs_left,	/*  Number of outputs left that need to be */
				/* declared as a certain type		   */
            *num_in_enums,	/*  Number of input enumerations	   */
            *num_out_enums,	/*  Number of output enumerations	   */
            *num_in_nodes,	/*  Number of input nodes for this input   */
            *num_out_nodes,	/*  Number of output nodes for this output */
            num_inputs,		/*  Number of inputs in the file  */
  	    num_outputs;	/*  Number of outputs in the file  */
  float     bin_pos,            /*  Value to assign positive binary values  */
            bin_neg;            /*  Value to assign negative binary values  */
  char      parse_table [NUM_COMMANDS][16];	/* Table of commands */
  enum_cvrt **input_table,	/*  Table of input enumerations  */ 
            **output_table;	/*  Table of output enumerations  */
  queue     data;		/*  Queue to hold the data points read  */
} parse_info;


/*  Function Prototypes  */

void  init_parse 	( char *, int, float, float,
                          net_info *, parse_info *, FILE **);
void  shutdown_parse	( parse_info *, FILE ** );
void  print_rosetta     ( parse_info );

char  *get_line  	( FILE * );

void  parse_line 	( char *, parse_info *, net_info * );
int   enum_line		( char *, parse_info * );

void  setup_mode	( parse_info * );
void  train_mode	( parse_info *, net_info * );
void  validation_mode	( parse_info *, net_info * );
void  test_mode		( parse_info *, net_info * );
void  gen_dataset       ( parse_info *, net_info * );
void  end_sequence      ( parse_info *, net_info * );
void  set_protocol	( char *, parse_info *, net_info * );
void  set_offset	( char *, parse_info *, net_info * );
void  set_inputs	( char *, parse_info *, net_info * );
void  set_outputs       ( char *, parse_info *, net_info * );

void  set_in		( char *, parse_info *, net_info * );
void  set_out           ( char *, parse_info * );
void  set_cont	        ( int * );
void  set_binary    	( float, float, int *, int *, enum_cvrt ** );
void  set_enum	        ( float, float, char *, int *, int *, enum_cvrt **, 
                          int );

void  data_line         ( char *, parse_info *, net_info * );
char  *get_token        ( char * );
void  lookup_token      ( char *, int, float *, enum_cvrt *, int, int * );

float  *get_equiv	( int, int, int, float, float );
int    num_bin_nodes	( int ); 
char   *itoa            ( int, char * );

void   parse_err	( int, char * );


/*	Global Variable Declaration	*/

static jmp_buf error_trap;	/*  Place keeper in case there is a problem  */
				/* with the data set.  This allows a         */
				/* graceful exit.			     */
unsigned long  lineNum;         /*  Used in case of error to allow the user  */
                                /* to spot the error in the data file        */


/*	PARSE -  This function takes the filename of the dataset, information
	on how to handle enumerations, and returns the parsed
	data in a structure.  This structure contains a series of input/output
	sets as well as information about the topography of network.
*/

int  parse  ( char *filename, int parameters, float bin_pos, float bin_neg,
              net_info *net_config )
{
  FILE       *datafile;		/*  This is the datafile being used */
  char       *line_in;		/*  The line read in from the data file */
  parse_info parse_data;	/*  Information needed to parse the file */
  int        err_no = 0;        /*  Parse error that was received  */

  /*  Mark place in case there is an error in the data set	*/  

  if  ( (err_no = setjmp (error_trap)) != 0 )  {
    if  ( err_no != 26 )  {
      shutdown_parse  ( &parse_data, &datafile );
      discard_net     ( net_config );
    }
    return FALSE;
  }


  /*  Initialize the parsing algorithm and open the data file  */

  init_parse  ( filename,  parameters, bin_pos, bin_neg, net_config, 
                &parse_data, &datafile );

  /*  Repeatedly read lines and then parse them  */

  while  (( line_in = get_line ( datafile ) ) != NULL )  {
    if  ( *line_in != '\0' )
      parse_line ( line_in, &parse_data, net_config );
    free( line_in );
  }
  gen_dataset  ( &parse_data, net_config );	/*  Generate the last data  */
						/* set that was read in     */

  print_rosetta  ( parse_data );
  shutdown_parse  ( &parse_data, &datafile );	/*  Clean up workspace  */
  
  return TRUE;
}  


/*	DISCARD_NET -  This function deallocates memory allocated to the
	net_info data structure and returns it to the system.  Call this
	function when you are completely done with the data sets, as they are
	disipated by it.
*/

void discard_net  ( net_info *net_config )
{
  if  ( net_config -> out_type != NULL )
    free ( net_config -> out_type );
  if  ( net_config -> train != NULL )
    free ( net_config -> train );
  if  ( net_config -> validate != NULL )
    free ( net_config -> validate );
  if  ( net_config -> test != NULL )
    free ( net_config -> test );

  net_config -> out_type  = NULL;
  net_config -> train     = NULL;
  net_config -> validate  = NULL;
  net_config -> test      = NULL;
}


/*	INIT_PARSE -  Initializes the data structures for the parse algorithm
	and opens the data file.
*/

void  init_parse  ( char *filename, int parameters, float bin_pos, 
                    float bin_neg, net_info *net_config, 
                    parse_info *parse_data, FILE **datafile )
{
  char parse_delim [NUM_COMMANDS][16] = {
    "$SETUP", "$TRAIN", "$VALIDATION", "$TEST",  "<>",
    "PROTOCOL:", "OFFSET:", "INPUTS:", "OUTPUTS:", "IN[", "OUT[" };

  int i;	/*  Index variable  */


  /*  Open the data file  */

  if  ( ( *datafile = fopen ( filename, "rt" ) ) == NULL )  {
    parse_err( 26, filename );
    return;
  }

  lineNum = 0;

  net_config -> protocol     = 0;	/*  Initialize the net configuration */
  net_config -> offset       = 0;	/* data structure.		     */
  net_config -> inputs       = 0;
  net_config -> outputs      = 0;
  net_config -> train_pts    = 0;
  net_config -> validate_pts = 0;
  net_config -> test_pts     = 0;
  net_config -> train_seg    = 0;
  net_config -> validate_seg = 0;
  net_config -> test_seg     = 0;
  net_config -> train        = NULL;
  net_config -> validate     = NULL;
  net_config -> test         = NULL;

  parse_data -> parse_mode    = 0;		/*  Initialize the parsing  */
  parse_data -> parameters    = parameters;	/* data structure	    */
  parse_data -> offset_left   = 0;
  parse_data -> inputs_left   = -1;
  parse_data -> outputs_left  = -1;
  parse_data -> num_in_enums  = NULL;
  parse_data -> num_out_enums = NULL;
  parse_data -> num_in_nodes  = NULL;
  parse_data -> num_out_nodes = NULL;
  parse_data -> num_inputs    = 0;
  parse_data -> num_outputs   = 0;  

  parse_data -> bin_pos = bin_pos;
  parse_data -> bin_neg = bin_neg;

  for  ( i = 0 ; i < NUM_COMMANDS ; i++ )
    strcpy ( parse_data -> parse_table [i], parse_delim [i] );

  parse_data -> input_table  = NULL;
  parse_data -> output_table = NULL;

  QUEUE_INIT( parse_data -> data );		/*  Initialize the queue  */
}


/*	PRINT_ROSETTA -  Prints a rosetta stone for inputs and outputs to
	aid in the deciphering of network data.
*/

void print_rosetta  ( parse_info parse_data )
{
  int i,j,k;

  fprintf (stderr, "\nRosetta Stone\n\n");
  
  for  (i=0;i<parse_data.num_inputs;i++)  {
    fprintf  (stderr,"Input %d-  ",i+1);
    if  ( parse_data.num_in_enums[i] == 0 )
      fprintf  (stderr,"cont\n");
    else
      for (j=0;j<parse_data.num_in_enums[i];j++)  {
	fprintf(stderr,"\n         %s :  ",parse_data.input_table[i][j].name);
	for  (k=0;k<parse_data.num_in_nodes[i];k++)
	  fprintf(stderr,"%4.2f ", parse_data.input_table[i][j].equiv[k]);
      }
    fprintf (stderr,"\n");
  }
  
  for  (i=0;i<parse_data.num_outputs;i++)  {
    fprintf  (stderr,"Output %d- ",i+1);
    if  ( parse_data.num_out_enums[i] == 0 )
      fprintf  (stderr,"cont\n");
    else
      for (j=0;j<parse_data.num_out_enums[i];j++)  {
	fprintf(stderr,"\n         %s :  ",parse_data.output_table[i][j].name);
	for  (k=0;k<parse_data.num_out_nodes[i];k++)
	  fprintf(stderr,"%4.2f ", parse_data.output_table[i][j].equiv[k]);
      }
    fprintf (stderr,"\n");
  }

  fprintf (stderr,"\n\n");
}

/*	SHUTDOWN_PARSE -  This function performs post-parsing cleanup.
	Basically, it deallocates memory that was allocated during the
	parsing of the data file, flushes the queue, and closes the data file
*/

void  shutdown_parse  ( parse_info *parse_data, FILE **datafile )
{
  int i;

  for  ( i = 0 ; i < parse_data -> num_inputs ; i++ )
    free ( *(parse_data -> input_table + i) );
  for  ( i = 0 ; i < parse_data -> num_outputs ; i++ )
    free ( *(parse_data -> output_table + i) );
  
  free ( parse_data -> num_in_enums );
  free ( parse_data -> num_out_enums );
  free ( parse_data -> num_in_nodes );
  free ( parse_data -> num_out_nodes );
  free ( parse_data -> input_table );
  free ( parse_data -> output_table );
  
  flush_queue ( &(parse_data -> data) );

  if  ( *datafile != NULL )
    fclose ( *datafile );
}


/*	GET_LINE -  This function reads data from the specified file until it
	reaches a segment delimiter or a semicolon.  It allocates memory and 
	returns this line, minus any whitespace.  It returns a pointer to the
	line, unless the end-of-file is reached, in which case it returns a
	NULL.  Note that if GET_LINE reaches an end-of-file when it is still
	looking for a semicolon, it returns an error.
*/

char *get_line  ( FILE *datafile )   
{
  char instr [256],	/*  This is the string as read from the data file  */
       temp  [256],	/*  The string minus the whitespace  */
       *line = NULL,	/*  This is the string to be returned  */
       delim_table [][16] =
	 { "$SETUP", "$TRAIN", "$VALIDATION", "$TEST", "<>" };
  int  inlen,		/*  The length of the line read in  */
       i, j;

  if  ( feof( datafile ) )	/*  Check for valid end of file  */
    return NULL;

  while  ( TRUE )  {
    if  ( feof( datafile ) )  /*  Check for invalid end-of-file  */
      if  ( *line == '\0' )
        return NULL;
      else
        parse_err( 1, NULL );

    if ( fgets ( instr, 256, datafile ) == NULL)	/*  Read the line  */
      return NULL;
    lineNum++;

    /*  Check for a special delimiter, and return it, if found  */

    for  ( i = 0 ; i < NUM_DELIM ; i++ )
      if  ( strstr ( instr, *(delim_table+i) ) == instr )  {
	if  ( (line = (char *)malloc (strlen(*(delim_table+i))+1)) == NULL )
	  NO_MEMORY;
	strcpy ( line, *(delim_table+i) );
	return line;
      }

    /*  Copy INSTR over to TEMP, removing whitespace as you go  */

    i = j = 0;
    inlen = strlen( instr );
    while  ( ( instr [i] != ';' ) && ( i < inlen ) )  {
      if  ( !isspace( instr [i] ) )
	temp [j++] = instr [i];
      i++;
    }
    temp [j] = '\0';

    /*  Allocate or Reallocate memory to the string and then copy TEMP over  */

    if  ( line == NULL )  {
      if  ( ( line = (char *) malloc ( strlen( temp ) + 1 ) ) == NULL )  
	NO_MEMORY;
      strcpy ( line, temp );
    } else {
      if  ( ( line = (char *) realloc ( line, strlen( line ) +
				        strlen( temp ) + 1 ) ) == NULL )  
	NO_MEMORY;
      strcat ( line, temp );
    }

    if  ( instr [i] == ';' )	/*  If a semicolon has been reached, return  */
      return line;		/* the line				     */
  }
}


/*	PARSE_LINE -  Parse line takes the line created by GET_LINE, retrieves
	the command from it, and then calls the appropriate function to parse
	it.
*/

void  parse_line  ( char *line, parse_info *parse_data, net_info *net_config )
{
  switch ( enum_line ( line, parse_data ) )  { 
    case 0  :  setup_mode ( parse_data );
	       break;
    case 1  :  train_mode ( parse_data, net_config );
               break;
    case 2  :  validation_mode ( parse_data, net_config );
               break;
    case 3  :  test_mode ( parse_data, net_config );
               break;
    case 4  :  end_sequence ( parse_data, net_config );
               break;
    case 5  :  set_protocol ( line, parse_data, net_config );
               break;
    case 6  :  set_offset ( line, parse_data, net_config );
               break;
    case 7  :  set_inputs ( line, parse_data, net_config );
               break;
    case 8  :  set_outputs ( line, parse_data, net_config );
               break;
    case 9  :  set_in  ( line, parse_data, net_config );
               break;
    case 10 :  set_out ( line, parse_data );
               break;
    default :  data_line  ( line, parse_data, net_config );
	       break;
  }
}


/*	ENUM_LINE -  This function takes a character string and retrieves any
	valid instructions from it.
*/

int enum_line  ( char *line, parse_info *parse_data )
{
  int i = 0; 	/*  Indexing variable  */

  while  ( i < NUM_COMMANDS )  {
    if ( strstr ( line, parse_data -> parse_table [i] ) == line )  {
      strcpy ( line, line+strlen( parse_data -> parse_table [i] ) - 1 );
      return i;
    }
    i++;
  }

  return -1;
}
  

/*	SETUP_MODE -  This function puts the parser into SETUP mode, checking
	first to see that the parser is in 'neutral', that is, not in any mode.
*/

void setup_mode ( parse_info *parse_data )
{
  if  ( parse_data -> parse_mode != 0 )		/*  Check for validity  */    
    parse_err  ( 2, NULL );

  parse_data -> parse_mode = 1;			/*  Switch modes  */
}


/*	TRAIN_MODE -  This function puts the parser into TRAIN mode.
*/

void train_mode ( parse_info *parse_data, net_info *net_config )
{
  int  i, j, k = 0;
  char message [5];	/*  Error message for parse_err  */

  /*  Check to make sure that everything that should be set, is set  */

  if  ( parse_data -> inputs_left != 0 )
    parse_err  ( 6, itoa( parse_data -> inputs_left, message ) );
  if  ( parse_data -> outputs_left != 0 )
    parse_err  ( 7, itoa( parse_data -> outputs_left, message ) );
  if  ( parse_data -> num_inputs == 0 )
    parse_err  ( 21, NULL );
  if  ( parse_data -> num_outputs == 0 )
    parse_err  ( 22, NULL );
  if  ( net_config -> protocol == 0 )
    parse_err  ( 23, NULL );

  /*  Check to make sure that we are in SETUP mode  */

  if  ( parse_data -> parse_mode != 1 )
    parse_err  ( 3, NULL );

  /*  Total the number of nodes needed for input and output and store that  */
  /* information in the net_config data structure.			    */

  net_config -> inputs = 0;
  for  ( i = 0 ; i < parse_data -> num_inputs ; i++ )
    net_config -> inputs += *( parse_data -> num_in_nodes + i );

  net_config -> outputs = 0;
  for  ( i = 0 ; i < parse_data -> num_outputs ; i++ )
    net_config -> outputs += *( parse_data -> num_out_nodes + i );

  /*  Calculate which nodes are binary and which are floating point  */

  if  ( ( (net_config -> out_type) = (int *)
           calloc ( net_config -> outputs, sizeof( int ) ) ) == NULL )
    NO_MEMORY;

  for  ( i = 0 ; i < parse_data -> num_outputs ; i++ )
    for  ( j = 0 ; j < parse_data -> num_out_nodes [i] ; j++ )
      if  ( parse_data -> output_table [i] == NULL )
        net_config -> out_type [k++] = CONT;
      else
        net_config -> out_type [k++] = BINARY;

  parse_data -> parse_mode = 2;			/*  Switch modes  */
}


/*	VALIDATION_MODE -  This function puts the parser into VALIDATION mode.
*/

void validation_mode ( parse_info *parse_data, net_info *net_config )
{
  if  ( parse_data -> parse_mode != 2 )		/*  Check for legality  */
    parse_err  ( 4, NULL );

  gen_dataset  ( parse_data, net_config );	/*  Generate the data set  */
						/* for the previous mode   */

  parse_data -> parse_mode = 3;			/*  Switch modes  */
}


/*	TEST_MODE -  This function puts the parser into TEST mode.
*/

void test_mode ( parse_info *parse_data, net_info *net_config )
{
  /*  Check to make sure that we were either in training or validation mode  */

  if ( (parse_data -> parse_mode != 2) && (parse_data -> parse_mode != 3) )
    parse_err  ( 5, NULL );

  gen_dataset  ( parse_data, net_config );	/*  Generate the data set  */
						/* for the previous mode   */

  parse_data -> parse_mode = 4;		/*  Switch modes  */
}


/*	GEN_DATASET -  This function takes the data points contained in the
	queue, and puts them into a data set.  The data set they are put into
	is determined by what mode the parser is in when the function is
	called.
*/

void gen_dataset ( parse_info *parse_data, net_info *net_config )
{
  int         i = 0,
              num_pts;		/*  The number of data points in this set  */
  data_point  *this_point;	/*  The last data point dequeued  */
  data_set    this_set;		/*  This is the set of data being worked on  */

  num_pts = (parse_data -> data).num_elem;	/*  Get the data count  */

  /*  Allocate memory for the data set  */

  if  ( ( this_set = (data_set)calloc (num_pts, sizeof( data_point ))) == NULL)
    NO_MEMORY;

  /*  Dequeue the data points and put them in the data set  */

  while  ( !QUEUE_ISEMPTY( parse_data -> data ) )  {
    dequeue_p  ( &(parse_data -> data), (void *) &this_point );
    *(this_set+i) = *this_point;
    i++;
  }

  /*  Determine which data set this is, and set it to that set  */

  if  ( parse_data -> parse_mode == 2 )  {
    net_config -> train_pts = num_pts;
    net_config -> train     = this_set;
  }  else if  ( parse_data -> parse_mode == 3 )  {
    net_config -> validate_pts = num_pts;
    net_config -> validate     = this_set;
  }  else  {
    net_config -> test_pts = num_pts;
    net_config -> test     = this_set;
  }
}


/*	END_SEQUENCE -  This function is called whenever an end-of-sequence
	marker has been reached.  It inserts a marker into the data point
	queue and resets the offset_left to it's origonal value.
*/

void end_sequence ( parse_info *parse_data, net_info *net_config )
{
  data_point this_point;

  if  ( net_config -> protocol != 2 )	/*  Check for sequence data set  */
    parse_err  ( 25, NULL );

  this_point.inputs  = NULL;	/*  Insert the marker  */
  this_point.outputs = NULL;

  /*  Insert the marker into the queue  */

  enqueue  ( &(parse_data -> data), &this_point, sizeof( data_point ) );

  /*  Reset the offset_left counter  */

  parse_data -> offset_left = net_config -> offset;

  /*  Increment segment counter  */

  if  ( parse_data -> parse_mode == 2 )
    net_config -> train_seg++;
  else  if  ( parse_data -> parse_mode == 3 )
    net_config -> validate_seg++;
  else
    net_config -> test_seg++; 
}


/*	SET_PROTOCOL -  This function determines what protocol the data file is
	in and then sets the parsing protocol.
*/

void set_protocol ( char *line, parse_info *parse_data, net_info *net_config )
{
  if  ( parse_data -> parse_mode != 1 )		/*  Check SETUP mode  */
    parse_err  ( 8, NULL );

  if  ( !strcmp ( line, ":IO" ) )	/*  Set protocol  */
    net_config -> protocol = 1;
  else if  ( !strcmp ( line, ":SEQUENCE" ) )
    net_config -> protocol = 2;
  else  	
    parse_err  ( 9, line + 1 );			/*  Unknown protocol  */
}


/*	SET_OFFSET -  This function sets the number of inputs, per segment, to
	read before an output is expected.
*/

void set_offset  ( char *line, parse_info *parse_data, net_info *net_config )
{
  if  ( !is_int( line + 1 ) )
    parse_err  ( 27, line + 1 );
  net_config -> offset = atoi( line + 1 );
  if  ( net_config -> offset < 0 )	/*  Negative offset  */
    parse_err  ( 10, NULL );
  parse_data -> offset_left = net_config -> offset;
}


/*	SET_INPUTS -  This function sets the expected number of inputs.  It
	also allocates memory in the parsing information table to record the
	information about each of these inputs.
*/

void set_inputs  ( char *line, parse_info *parse_data, net_info *net_config )
{
  int i;	/*  Index variable  */

  /*  Read in the number of inputs  */
  
  if  ( !is_int( line+1 ) )
    parse_err ( 27, line+1 );
  parse_data -> num_inputs = atoi( line+1 );
  if  ( parse_data -> num_inputs < 1 )
    parse_err  ( 11, NULL );		/*  Negative inputs  */

  /*  Set the number of inputs expected to the total number of inputs  */

  parse_data -> inputs_left = parse_data -> num_inputs;
  
  /*  Allocate memory for the parsing information  */

  if  ( ( parse_data -> num_in_nodes = 
          (int *) calloc ( parse_data -> num_inputs, sizeof( int ) )) == NULL )
    NO_MEMORY;

  if  ( ( parse_data -> input_table = (enum_cvrt **) 
          calloc ( parse_data -> num_inputs, sizeof( enum_cvrt * ) )) == NULL )
    NO_MEMORY;

  if  ( ( parse_data -> num_in_enums = 
          (int *) calloc ( parse_data -> num_inputs, sizeof( int ) )) == NULL )
    NO_MEMORY;

  /*  Reset the information in the parsing table that was just allocated  */

  for  ( i = 0 ; i < parse_data -> num_inputs ; i++ )  {
    *( parse_data -> num_in_nodes + i ) = 0;
    *( parse_data -> input_table + i )  = NULL;
    *( parse_data -> num_in_enums + i ) = 0;
  } 
}


/*	SET_OUTPUTS -  Sets the number of outputs and allocates memory for
	information about each one in the parsing table.
*/

void set_outputs  ( char *line, parse_info *parse_data, net_info *net_config )
{
  int i;	/*  Index variable  */

  /*  Read the number of inputs from the line  */

  if  ( !is_int( line+1 ) )
    parse_err ( 27, line+1 );
  parse_data -> num_outputs = atoi( line+1 );
  if  ( parse_data -> num_outputs < 1 )
    parse_err  ( 12, NULL );			/*  Negative outputs  */

  /*  Set the number of outputs left to equal the total number of outputs  */

  parse_data -> outputs_left = parse_data -> num_outputs;

  /*  Allocate memory to store the information about the outputs  */

  if  ( ( parse_data -> num_out_nodes = 
         (int *) calloc ( parse_data -> num_outputs, sizeof( int ) )) == NULL )
    NO_MEMORY;

  if  ( ( parse_data -> num_out_enums = 
         (int *) calloc ( parse_data -> num_outputs, sizeof( int ) )) == NULL )
    NO_MEMORY;

  if  ( ( parse_data -> output_table = (enum_cvrt **)
         calloc ( parse_data -> num_outputs, sizeof( enum_cvrt * ) )) == NULL )
    NO_MEMORY;

  /*  Initialize the information in the output tables  */

  for  ( i = 0 ; i < parse_data -> num_outputs ; i++ )  {
    *( parse_data -> num_out_nodes + i ) = 0;
    *( parse_data -> num_out_enums + i ) = 0;
    *( parse_data -> output_table + i ) =  NULL;
  }
}


/*	SET_IN -  This function determines the type of an input.  First it
	determines what input is being set and then checks to see if that
	input is already set.  If it isn't, it is set to the type specified
	in the data file.
*/

void set_in  ( char *line, parse_info *parse_data, net_info *net_config )
{
  int   in_num;		/*  The number of the input being set  */
  char  message [5],	/*  Parse error message  */
        *num_str;       /*  String representing the number  */
  
  /*  Read in the input number and check it for validity  */

  num_str = strtok ( strchr( line, '[' )+1, "]" );
  if  ( !is_int( num_str ) )
    parse_err ( 27, num_str );
  in_num = atoi( num_str );


  if  ( *( parse_data -> num_in_nodes + in_num - 1 ) != 0 )
    parse_err ( 13, itoa( in_num, message ) );	/*  Input already set  */
  ( parse_data -> inputs_left )--;
  strcpy ( line, line + 3 + strlen( num_str ) );  

  /*  Classify the input and then call a function to set up the necessary  */
  /* nodes								   */

  if  ( !strncmp ( line, "CONT", 4 ) )
    set_cont  ( parse_data -> num_in_nodes + in_num - 1 );
  else  if ( !strcmp ( line, "BINARY" ) )
    set_binary  ( parse_data -> bin_pos, parse_data -> bin_neg,
                  parse_data -> num_in_enums + in_num - 1,
                  parse_data -> num_in_nodes + in_num - 1,
                  parse_data -> input_table + in_num - 1 );
  else  if ( !strncmp ( line, "ENUM", 4 ) )
    set_enum  ( parse_data -> bin_pos, parse_data -> bin_neg,
                line + 5, parse_data -> num_in_enums + in_num - 1,
                parse_data -> num_in_nodes + in_num - 1,
                parse_data -> input_table + in_num - 1,
                parse_data -> parameters & BINARY_IN );
  else
    parse_err  ( 14, line );	/*  Unknown input type  */
}


/*	SET_OUT -  This function operated similarly to the SET_IN function,
	except that it is used to set outputs instead of inputs.  The major
	difference between these two functions are the data structures they
	modify and the fact that SET_OUT does not modify ALL_ENUM.
*/

void  set_out  ( char *line, parse_info *parse_data )
{
  int  out_num,		/*  Output being set  */
       dummy;
  char message [5],	/*  Error message  */
       *num_str;        /*  String actually representing the number  */

  /*  Get the output number being set and make sure it hasn't already  */

  num_str = strtok ( strchr( line, '[' )+1, "]" );
  if  ( !is_int( num_str ) )
    parse_err ( 27, num_str );
  out_num = atoi( num_str );

  if  ( *( parse_data -> num_out_nodes + out_num - 1 ) != 0 )
    parse_err  ( 16, itoa( out_num, message ) );  /*  Resetting an output  */
  ( parse_data -> outputs_left )--;
  strcpy ( line, line + 3 + strlen( num_str ) );

  /*  Classify the output and call a function to deal with the implications  */

  if  ( !strncmp ( line, "CONT", 4 ) )
    set_cont  (  parse_data -> num_out_nodes + out_num - 1 );
  else  if  ( !strcmp ( line, "BINARY" ) )
    set_binary ( parse_data -> bin_pos, parse_data -> bin_neg,
                 parse_data -> num_out_enums + out_num - 1,
	 	 parse_data -> num_out_nodes + out_num - 1,
		 parse_data -> output_table + out_num - 1 );
  else  if  ( !strncmp ( line, "ENUM", 4 ) )
    set_enum  ( parse_data -> bin_pos, parse_data -> bin_neg,
                line + 5, parse_data -> num_out_enums + out_num - 1,
    		parse_data -> num_out_nodes + out_num - 1,
		parse_data -> output_table + out_num - 1,
		parse_data -> parameters & BINARY_OUT );
  else
    parse_err  ( 14, line );	/*  Unknown type  */
}


/*	SET_CONT -  This function sets up a continuous node.
*/

void  set_cont  ( int *num_nodes )
{
  *num_nodes = 1;
}


/*	SET_BINARY -  This function sets up the node for a binary input/output.
*/

void  set_binary  ( float bin_pos, float bin_neg, 
                    int *num_enums, int *num_nodes, enum_cvrt **table )
{
  *num_enums = 2;    /*  Set information about the node  */
  *num_nodes = 1;
  
  /*  Allocate memory for the enumeration table  */

  if  ( (*table = (enum_cvrt *) calloc (2,sizeof( enum_cvrt ))) == NULL )
    NO_MEMORY;

  /*  Allocate memory for the affirmative and copy info into that memory  */

  if  (( (*table) -> name = (char *) calloc ( 2, sizeof( char ) )) == NULL )
    NO_MEMORY;
  if  (( (*table) -> equiv = (float *) malloc ( sizeof( double ) ))  == NULL )
    NO_MEMORY;
  strcpy  ( (*table) -> name, "+" );
  *((*table) -> equiv) = bin_pos;

  /* Allocate memory for the negative and copy info into that memory  */

  if  (( (*table + 1) -> name = (char *)calloc ( 2, sizeof( char ) )) == NULL )
    NO_MEMORY;
  if  (( (*table + 1) -> equiv = (float *)malloc ( sizeof( double ))) == NULL)
    NO_MEMORY;
  strcpy  ( (*table + 1) -> name, "-" );
  *((*table + 1 ) -> equiv) = bin_neg;
}


/*	SET_ENUM -  This function sets up the nodes required for an enumerated
	input/output.
*/

void  set_enum  ( float bin_pos, float bin_neg, char *line, int *num_enum, 
                  int *num_nodes, enum_cvrt **table, int binary )
{
  queue enumerations;	/*  This queue contains a list of the enumerations  */
  char *tok;		/*  This si the last token read in  */
  int  type,		/*  This is set to '0' for binary representations */
			/* and '1' for unary ones.			  */
       i;


  QUEUE_INIT  ( enumerations );		/*  Initialize the queue  */

  /*  Get the first token  */

  if ( (tok = strtok ( line, "," )) == NULL )
    parse_err ( 15, NULL );	/*  No token  */
  enqueue  ( &enumerations, tok, ( strlen( tok ) + 1 ) * sizeof( char ) );
  
  /*  Get all succeeding tokens  */

  while  ( ( tok = strtok ( NULL, "," ) ) != NULL )  {
    if  ( strstr ( tok, "}" ) != NULL )
      *(tok + strlen( tok ) - 1) = NULL;
    enqueue  ( &enumerations, tok, ( strlen( tok ) + 1 ) *  sizeof( char ) );
  } 
  
  /*  Determine the number of nodes needed for these enumerations  */

  *num_enum = enumerations.num_elem;
  if  ( binary )  {
    *num_nodes = num_bin_nodes ( enumerations.num_elem - 1);
    type = 0;
  }  else  {
    *num_nodes = enumerations.num_elem;
    type = 1;
  }

  /*  Allocate memory for the conversion table  */

  if  (( *table = (enum_cvrt *)calloc (*num_enum,sizeof(enum_cvrt)) ) == NULL )
    NO_MEMORY;

  /*  Dequeue each enumeration and assign it a series of values  */

  for  ( i = 0 ; i < *num_enum ; i++ )  {
    dequeue_p  ( &enumerations, (void *) &tok );
    (*table + i) -> name = tok;
    (*table + i) -> equiv = get_equiv  ( i, *num_enum, type, 
                                         bin_pos, bin_neg );
  }
}


/*	NUM_BIN_NODES -  This function takes a number of enumerations, and
	determines how many nodes are needed for a binary representation.
*/

int  num_bin_nodes  ( int num_enum )
{
  int i = 0;

  while ( num_enum > 0 )  {
    i++;
    num_enum >>= 1;
  }

  if ( i > 1 )
    return i;
  else
    return 1;
}


/*	GET_EQUIV -  This function takes an index (representing which
	enumeration), the number of nodes used by this enumeration and the
	type of representation to use, and returns a string of floating point
	numbers to represent the enumeration.
*/

float *get_equiv  ( int index, int num_nodes, int type, float bin_pos, 
                    float bin_neg )
{
  float *result;	/*  This is the result to return to the caller  */
  int i;

  /*  Allocate memory for the array of floating point numbers  */

  if  ( (result = (float *) calloc ( num_nodes, sizeof( float ) )) == NULL )
    NO_MEMORY;

  /*  Determine the floating point sequence  */

  for  ( i = 0 ; i < num_nodes ; i++ )
    if  ( type )  {
      if  ( i == index )
        *(result + i) = bin_pos;
      else
        *(result + i) = bin_neg;
    } else {
      if  ( index & 1 )
        *(result + i) = bin_pos;
      else
        *(result + i) = bin_neg;
      index >>= 1;
    }
  
  return result;	/*  Return the sequence  */
}


/*	DATA_LINE -  This function is used to parse the individual data lines
	into data points, which are inserted into a queue to be used later to
	generate a data set.
*/

void  data_line  ( char *line, parse_info *parse_data, net_info *net_config )
{
  int i,
      num;
  float  *inputs,	/*  These are the inputs and outputs of the data  */
	 *outputs;	/* point					  */
  char *token,		/*  This is the token being parsed  */
       *sep;            /*  Location of the marker seperating inputs and  */
                        /* outputs.                                       */
  data_point this_data;	/*  This is the data point to be placed in the queue */

  /*  Check for a valid number of inputs  */

  if  ( (parse_data -> offset_left) == 0 )  {
    if  ( (sep = strstr( line, "=>" )) == NULL )
      parse_err ( 29, line );
  } else {
    sep = line + strlen( line );
  }
  num = count_char( line, ',', sep );
  num++;
  if  ( num < (parse_data -> num_inputs) )
    parse_err ( 30, NULL );
  if  ( num > (parse_data -> num_inputs) )
    parse_err ( 31, NULL );

  /*  Allocate memory for the data points  */

  if  (( inputs = (float *) 
           calloc  ( net_config -> inputs, sizeof( float ) )) == NULL )
    NO_MEMORY;
  if  (( outputs = (float *)
           calloc  ( net_config -> outputs, sizeof( float ) )) == NULL )
    NO_MEMORY;

  /*  Read in the inputs and look them up in the conversion table  */

  for  ( i = 0 ; i < parse_data -> num_inputs; i++ )  {
    token = get_token ( line );
    lookup_token  ( token, i, inputs, *(parse_data -> input_table+i),
                    *(parse_data -> num_in_enums+i), 
                    parse_data -> num_in_nodes );
    free ( token );
  } 

  /*  If we are past the offset, read in the outputs and look them up in  */
  /* their conversion table.  Otherwise, just decrement OFFSET_LEFT.      */

  if  ( parse_data -> offset_left == 0 )  {
    num = count_char ( line, ',', line + (strlen( line )*sizeof( char )) ) + 1;
    if  ( num < (parse_data -> num_outputs) )
      parse_err ( 32, NULL );
    if  ( num > (parse_data -> num_outputs) )
      parse_err ( 33, NULL );
    for  ( i = 0 ; i < parse_data -> num_outputs ; i++ )  {
      token = get_token ( line );
      lookup_token  ( token, i, outputs, *(parse_data -> output_table+i),
                      *(parse_data -> num_out_enums+i), 
                      parse_data -> num_out_nodes );
      free ( token );
    }
  } else
    (parse_data -> offset_left)--;

  this_data.inputs = inputs;	/*  Set THIS_DATA to point to these values  */
  this_data.outputs = outputs;

  /*  Place the data point in the queue  */

  enqueue ( &(parse_data -> data), &this_data, sizeof( data_point ) );
}


/*	GET_TOKEN -  This function returns the first token it finds in LINE.
	A token is any character string seperated by ','s or '=>'s.
*/

char  *get_token    ( char *line )
{
  int  i = 0,
       j;
  char *tok;	/*  This is the token  */

  /*  Search for a delimiter  */

  while  ( (*(line + i) != '\0') && (*(line + i) != ',') &&
           (*(line + i) != '=') )
    i++;

  /*  If there is no token, return an error  */

  if  ( !i )
    parse_err  ( 20, NULL );

  /*  Allocate memory for the token and copy the token to that memory  */
  
  if  ( ((*(line + i) == '=' ) && (*(line + i + 1) == '>')) ||
        (*(line + i) == ',' )  || (*(line + i) == '\0') )  {
    if  ( (tok = (char *)calloc (i+1, sizeof( char ))) == NULL )
      NO_MEMORY;
    for  ( j = 0 ; j < i ; j++ )
      *(tok+j) = *(line+j);
    *(tok+i) = '\0';
    strcpy ( line, line + i + 1 + (*(line+i) == '=') );
  } else
    parse_err  ( 20, NULL );

  return tok;	/*  Return the token  */
}


/*	LOOKUP_TOKEN -  This function looks up the token passed to it in the
	conversion table passed to it and inserts the equivelence into the
	translation, at the point specified.
*/
 
void  lookup_token  ( char *token, int position, float *translation,
                      enum_cvrt *enum_table, int num_enums, int *num_nodes )
{
  int found = FALSE,
      advance = 0,	/*  Number of positions to advance the start of the */
			/* equivelence string				    */
      i = 0,
      j;

  for  ( i = 0 ; i < position ; i++ )
    advance += num_nodes [i];

  if  ( num_enums == 0 )  {			/*  If this is a CONT, just  */
    if  ( !is_float( token ) )                  /* insert it into the        */
      parse_err ( 28, token );                  /* translation               */
    translation[advance] = atof( token );
    return;
  }


  /*  This code has been added as a result of the admissions project here at */
  /* CMU.  It allows for unknowns among enumerated and binary data types.    */
  /* By placing an asterisk as the value of such a data item, you cause all  */
  /* nodes associated to that input/output to be set to '0.0'.  This is the  */
  /* least confusing (to the network) method that seems generally applicable */
  /* and readily available.                                                  */

  if  ( token [0] == '*' )  {
    for  ( j = 0 ; j < num_nodes [position] ; j++ )
      translation [advance+j] = 0.0;
    return;
  }

  /*  Search for the token  */

  i = 0;
  while  ( !found && ( i < num_enums ) )  {
    if  ( !strcmp ( token, (enum_table + i) -> name ) )
      found = TRUE;
    else
      i++;
  }


  if  ( !found )
    parse_err  ( 19, token );	/*  Token not found  */
  

  /*  Insert the equivelence into the translation array  */

  for  ( j = 0 ; j < num_nodes [position] ; j++ )
    translation [advance+j] = enum_table [i].equiv [j];
}


/*	ITOA -  This function takes a number and inserts it into a string. A
	pointer to this string is returned to the calling process.
*/

char *itoa  ( int num, char *str )
{
  sprintf  ( str, "%d", num );
  return str;
}


/*	PARSE_ERR -  This function whenever an error has been reached in the
	data file.  It prints out an error message and then exits from the 
	program.
*/

void  parse_err  ( int err_no, char *message )
{
  fprintf  ( stderr, "\n\nParse Error (#%d : line %d)- ", err_no, lineNum );

  switch  ( err_no )  {
    case 1  :  fprintf ( stderr, "Unexpected end-of-file" );
	       break;
    case 2  :  fprintf ( stderr, "Illegal attempt to reenter SETUP mode" );
               break;
    case 3  :  fprintf ( stderr, "Illegal attempt to enter TRAIN mode" );
               break;
    case 4  :  fprintf ( stderr, "Illegal attempt to enter VALIDATION" );
               fprintf ( stderr, " mode" );
               break;
    case 5  :  fprintf ( stderr, "Illegal attempt to enter TEST mod" );
               break;
    case 6  :  fprintf ( stderr, "Leaving SETUP mode with %s", message );
               fprintf ( stderr, " inputs undefined" );
               break;
    case 7  :  fprintf ( stderr, "Leaving SETUP mode with %s", message );
               fprintf ( stderr, " outputs undefined" );
               break;
    case 8  :  fprintf ( stderr, "Attempt to set protocol outside of SETUP");
               fprintf ( stderr, " mode" );
               break;
    case 9  :  fprintf ( stderr, "Unknown protocol: %s", message );
               break;
    case 10 :  fprintf ( stderr, "Negative offset is not allowed" );
               break;
    case 11 :  fprintf ( stderr, "Less than one input is not allowed" );
               break;
    case 12 :  fprintf ( stderr, "Less than one output is not allowed" );
               break;
    case 13 :  fprintf ( stderr, "Input #%s redefined", message );
               break;
    case 14 :  fprintf ( stderr, "Unknown node type: %s", message );
               break;
    case 15 :  fprintf ( stderr, "Enumerated nodes must have at least 1" );
               fprintf ( stderr, " enumeration" );
               break;
    case 16 :  fprintf ( stderr, "Output #%s redefined", message );
               break;
    case 17 :  fprintf ( stderr, "No input where expected" );
               break;
    case 19 :  fprintf ( stderr, "Undeclared enumeration: %s", message );
               break;
    case 20 :  fprintf ( stderr, "Token not found in data line" );
               break;
    case 21 :  fprintf ( stderr, "No inputs specified" );
               break;
    case 22 :  fprintf ( stderr, "No outputs specified" );
               break;
    case 23 :  fprintf ( stderr, "No protocol specified" );
               break;
    case 24 :  fprintf ( stderr, "No training points" );
               break;
    case 25 :  fprintf ( stderr, "Sequence-end marker found in " );
               fprintf ( stderr, "non-sequence data set" );
               break;
    case 26 :  fprintf ( stderr, "Unable to open data file: %s", message );
               break;
    case 27 :  fprintf ( stderr, "Data item should be an integer: %s", 
			 message );
               break;
    case 28 :  fprintf ( stderr, "Data item should be a floating point");
               fprintf ( stderr, " or integer value: %s", message );
               break;
    case 29 :  fprintf ( stderr, "No input seperator on data line: %s",
			 message );
               break;
    case 30 :  fprintf ( stderr, "Too few inputs on data line" );
               break;
    case 31 :  fprintf ( stderr, "Too many inputs on data line" );
               break;
    case 32 :  fprintf ( stderr, "Too few outputs on data line" );
               break;
    case 33 :  fprintf ( stderr, "Too many outputs on data line" );
               break;
    default :  fprintf  ( stderr, "Unknown Error" );
               break;
  }

  fprintf  ( stderr, "\n\n" );
  longjmp  ( error_trap, err_no );
}
