/*	Extended C Toolbox Routines

	v1.0
	Matt White  (mwhite+@cmu.edu)
	6/23/93

	This library contains a series of simple functions, written in ANSI C,
	that extend the data types, file management, string handling and memory
        management capabilities of C.  Descriptions on how to use each function
	precede the code for that function.  Any questions should be directed
	to Matt White (mwhite+@cmu.edu).
*/


/*	Include Files	*/  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tools.h"
#include "queue.h"


/*	Data Structure Definitions	*/

/*	HELP_REC_T -  Holds the data associated with a single help topic
*/

typedef struct  {
  char *name;		/*  Name of the help topic  */
  char **descript;	/*  The actual help data for that topic  */
  int  Nlines;		/*  Number of lines of help available on topic  */
}  help_rec_t;


/*	HELP_T -  Holds all the data associated with the help for this program,
	including the topic information and the number of topics.
*/

typedef struct  {	
  help_rec_t  *topics;		/*  An array of topics, with their info  */
  int         Ntopics;		/*  The number of topics available  */
}  help_t;


/*	Global Variable Declarations	*/

help_t help_data;	/*  This is the data on the help for this program  */


/*	Constant Declarations		*/

#define NOT_FOUND -1	/*  If an item is not found, return this constant  */


/*	Function Prototypes		*/

int	compare_elem	( const void *, const void * );
int	find_help	( char * );
void	list_help	( void );


/*	ALLOC_MEM -  This function is similar to the C library routine, calloc
	except that it checks as to whether memory was successfully allocated
	or not.  If memory was NOT allocated, this function aborts the
	program, informing the user where memory was not allocated.

	Nelem		-  Number of elements to allocate.  If you are 
			   allocating memory for an array, put the number of 
			   elements here.
	elemSize	-  The size of each element.  This can be obtained with
			   a sizeof( ) instruction.
	callingFn	-  The name of the calling function.  In the case that
			   memory is not allocated, this name is displayed with
			   the error message.

	RETURNS:	A pointer to the allocated memory.
*/

void *alloc_mem  ( int Nelem, int elemSize, char *callingFn )
{
  void *retVal;		/*  This is the pointer to return  */

  /*  Allocate memory for the new pointer.  If insufficient memory is
      available, abort with an error message.				*/

  if  ( ( retVal = (void *) calloc ( Nelem, elemSize ) ) == NULL )  {
    fprintf  ( stderr, "\n***ERROR: Unable to allocate memory.\n");
    fprintf  ( stderr, "          Function: %s\n\n", callingFn );
    abort( );
  }

  return retVal;	/*  Return a pointer to the allocated memory  */
}


/*	BTOA -  Boolean to Ascii conversion.  This function returns an ascii
	representation of a boolean value.

	inVal		-  The boolean value to convert.
	outType		-  The type of representation desired.  This can be an
			   integer from 1 to 3.  A '1' indicates TRUE/FALSE,
			   '2' indicates YES/NO and '3' indicates ON/OFF.

	RETURNS:	A pointer to a character string.  Note that this is a
			static representation of the value.  If you wish to
			alter the string in any way, copy it over into another
			string first, and then make any modifications.
			Otherwise, all subsequent calls to this function will
			return the modified value.
*/

char *btoa  ( boolean inVal, int outType )
{
  /*  Array of character strings with the representations in it		*/
  static char *boolStrings [] = { "False", "True", "No", "Yes", "Off", "On" };

  /*  Return a pointer to the representation desired.  */
  return  ( boolStrings [2*(outType-1)+inVal] );
}


/*	ATOB -  Ascii to Boolean conversion.  This function takes a pointer to
	an ascii token and returns the boolean value that it represents.  If
	the ascii token does not correspond to any boolean value, a FALSE value
	is returned.  Note that the ascii token is first converted to upper
	case before any conversion is performed.  This has the side effect that
	the origonal string will be altered.

	token		-  Character token to convert.

	RETURNS:	The boolean representation of the string.
*/

boolean atob  ( char *token )
{
  str_upper( token );

  if  ( !strcmp( "TRUE", token ) || !strcmp( "YES", token ) || 
        !strcmp( "ON", token ) )
    return TRUE;
  return FALSE;
}


/*	STR_UPPER -  Converts a character string to upper case.  This function
	takes a pointer to a character string and converts that string to upper
	case.  Modification is performed on the origonal string.

	token		-  The string to be converted to upper case.  This
			   token IS modified by the function.

	RETURNS:	Nothing.
*/

void str_upper  ( char *token )
{
  int i;	/*  Indexing variable  */
  
  /*  Cycle through each character in the string, check to see if it's lower
      case, and then convert it to upper case if it is.			     */

  for  ( i = 0 ; i < strlen( token ) ; i++ )
    if  ( islower( token [i] ) )
      token [i] = toupper( token [i] );
}


/*	IS_INT	- Checks whether a string represents an integer.  Returns a
	true value if it does and false if not.

	inVal	- A pointer to the string to check.

	RETURNS:	TRUE if inVal represents an integer, FALSE if not.
*/

boolean	is_int  ( char *inVal )
{
  int len,
      i;

  if  ( (inVal == NULL) || ( (len = strlen( inVal )) < 1) )
    return FALSE;
  if  ( !isdigit( inVal [0] ) && (inVal [0] != '-') && (inVal [0] != '+') )
    return FALSE;

  for  ( i = 1 ; i < len ; i++ )
    if  ( !isdigit( inVal [i] ) )
      return FALSE;
  return TRUE;
}


/*	IS_FLOAT	- Checks whether the inputted string represents a
	floating point number.

	inVal	- This is the character string to check.

	RETURNS:	TRUE if the string represents a valid floating point
			number, FALSE otherwise. 
*/

boolean	is_float  ( char *inVal )
{
  int len,
      i;

  if  ( (inVal == NULL) || ( (len = strlen( inVal )) < 1) )
    return FALSE;
  if  ( !isdigit( inVal [0] ) && (inVal [0] != '-') && (inVal [0] != '+') &&
	(inVal [0] != '.') )
    return FALSE;

  for  ( i = 1 ; i < len ; i++ )
    if  ( !isdigit( inVal [i] ) && (inVal [i] != '.') )
      return FALSE;
  return TRUE;
}


/*	COUNT_CHAR -  Counts the number of occurences of a specific character
	in a string.  Does not modify the string.

	inStr	-  Pointer to the string to check.
	chkCh	-  Character to check for.
	stop	-  Pointer to a place in the string to stop.

	RETURNS	-  The number of occurences of chkCh in inStr.
*/

int	count_char  ( char *inStr, char chkCh, char *stop  )
{
  char   *index;
  int    count = 0;

  index = inStr;
  while ( index < stop )  {
    if  ( *index == chkCh )
      count++;
    index++;
  }

  return count;
}


/*	FILE_EXIST -  Checks to see whether a specified file exists.  There
	are no known side effects.

	fileName	-  The name of the file to check.  No modification is
			   performed on this string.

	RETURNS:	TRUE if the file exists, FALSE if it doesn't.
*/

boolean file_exist  ( char *fileName )
{
  FILE *dummy;		/*  Dummy file pointer  */

  /*  Check to see if the file is there by attempting to open it for a read.
      If unable to open the file, assume that it doesn't exist and return a
      false value.							     */

  if  ( ( dummy = fopen ( fileName, "r" ) ) == NULL )
    return FALSE;

  /*  File exists, so close the dummy and return a TRUE value.		*/

  fclose( dummy );
  return TRUE;
}


/*	GET_YN -  This function gets a Yes/No response from stdin.  Only the
	first character is read, all others are ignored.  If the character read
	is not either a Y or N (case insensitive), then a default value is
	returned.

	def		-  This is the value to return in the event that a
			   valid value is NOT read from the keyboard.

	RETURNS:	A boolean value representing the choice made by the
			user.
*/

boolean get_yn  ( boolean def )
{
  char inChar,		/*  The character read from stdin.  */
       dummy [2];

  inChar = getchar( );	/*  Read the input from stdin  */  

  if  ( ( inChar == 'Y' ) || ( inChar == 'y' ) )  {	/*  Check for a true */
    gets ( dummy );					/* or false value    */
    return YES;	 					/* and return an     */
  }							/* appropriate value */
  if  ( ( inChar == 'N' ) || ( inChar == 'n' ) )  {	/* after flushing    */
    gets ( dummy );					/* the input buffer  */
    return NO;
  }

  return def;		/*  Return the default  */
} 


/*	ADD_EXT -  This function takes the pointer to a file name and then
	searches through a list of extensions, removing any that it finds.
	After all extensions have been removed, another extension specified
	is added to the end of the file name, which is returned as outFile.
	There are no known side effects to this function.

	inFile		-  This is the file name to use as the base for the 
			   new file.  It is NOT modified by this function.
	outFile		-  This is a character string to store the results of
			   the function.  Memory for this string MUST be 
			   allocated prior to calling this function.
	ext		-  This is the extension to add to the original file
			   name.  Note that no '.' is added, so if you want
			   that, put it at the beginning of your extension.
	extList		-  This is an array of character strings, each an
			   extension to remove from the filename.  Again, the
			   '.' is NOT assumed.
	Next		-  This simply the number of extensions provided in
			   extList.

	RETURNS:	Nothing.
	NOTES:		If you do not want to remove a current extension, pass
			a NULL pointer in extList, and set Next to '0'.
*/
 
void  add_ext  ( char *inFile, char *outFile, char *ext, char *extList [],
                 int  Next )
{
  char *loc;	/*  Location of the start of the old extension  */
  int  i;	/*  Indexing variable  */

  strcpy ( outFile, inFile );	/*  Copy old file name into new buffer  */

  /*  Locate any current extensions and remove them by inserting a NULL
      character at their beginning.					  */

  for  ( i = 0 ; i < Next ; i++ )
    if  ( (loc = strstr ( outFile, extList [i] )) != NULL )
      *loc = NULL;

  strcat ( outFile, ext );	/*  Add the new extension to the file name  */
}


/************************** Help Functions ***********************************/

/*	Help Files
	~~~~~~~~~~
	A help file is a series of topics, followed by one or more lines of
	information on that topic.  A topic should be capitalized and
	preceded by a '$'.  All lines following that topic are considered part
	of that topic until another topic or the end-of-file is reached.
	
	Generalized help (i.e. help with no topic) should have the topic of
	'$HELP'.  A line can be designated as a comment line by preceding it
	with a '#' sign.  Note that both the topic and comment markers must
	be the first characters on their lines.
*/

/*	INIT_HELP -  Initialize the help data structure with information from
	the help file specified.  The file should have the format specified
	above.  Returns a TRUE value if the file was successfully read in, a
	FALSE otherwise.
*/

boolean  init_help  ( char *helpfile )
{
  FILE		*datafile;		/*  File pointer to help file	*/
  long		topicPos,
  		Nlines,
		Ntopics,
		i,j;
  char		inBuffer [81],
		*fn = "Initialize Help";
  boolean	topicStarted;
  queue		startPos,
		topicLen,
		topics;

  QUEUE_INIT( startPos );
  QUEUE_INIT( topicLen );
  QUEUE_INIT( topics );
  topicStarted = FALSE;

  /*  Check for help file and then open it  */

  if  ( !file_exist( helpfile ) )  {
    fprintf  ( stderr, "Unable to open help file.  Help not available.\n" );
    return FALSE;
  }

  datafile  = fopen ( helpfile, "rt" );


  /*  Find the start of every topic in the help file and record it.  Also  */
  /* record the title of each topic, and the number of lines it has.       */
  /* Ignore commented lines						   */

  while  ( !feof( datafile ) )  {
    if  ( fgets ( inBuffer, 81, datafile ) == NULL )
      break;
    if  ( inBuffer [0] == '$' )  {
      topicPos = ftell ( datafile );
      enqueue  ( &startPos, &topicPos, sizeof( long ) );
      if  ( topicStarted )
        enqueue  ( &topicLen, &Nlines, sizeof( long ) );
      else
        topicStarted = TRUE;
      inBuffer [strlen( inBuffer ) - 1] = '\0';
      enqueue  ( &topics, inBuffer + 1,(strlen( inBuffer )+1)*sizeof( char ) );
      Nlines = 0;
    }  else if  ( inBuffer [0] != '#' )  {
      Nlines++;
    } 
  }
  enqueue  ( &topicLen, &Nlines, sizeof( long ) );

  /*  Allocate memory for help data  */

  Ntopics 	   = startPos.num_elem;
  help_data.topics = (help_rec_t *)alloc_mem ( Ntopics, sizeof( help_rec_t ),
					       fn );
  help_data.Ntopics = Ntopics;

  /*  Go through and read in the information for each topic, using the  */
  /* information gathered above.					*/

  for  ( i = 0 ; i < Ntopics ; i++ )  {
    j = 0;
    dequeue   ( &topicLen, &Nlines );
    dequeue_p ( &topics, (void **) &(help_data.topics [i].name) );
    dequeue   ( &startPos, &topicPos );
    fseek     ( datafile, topicPos, 0 );
    help_data.topics [i].descript = (char **)alloc_mem(Nlines,sizeof( char * ),
						       fn );
    help_data.topics [i].Nlines = Nlines;
    while  ( j < Nlines )  {
      help_data.topics [i].descript [j] = (char *)alloc_mem ( 81, 
						          sizeof( char ), fn );
      do  {
        fgets ( help_data.topics [i].descript [j], 81, datafile );
      }  while  ( help_data.topics [i].descript[j][0] == '#' );
      j++;
    }
  }
  fclose  ( datafile );

  qsort  ( help_data.topics, help_data.Ntopics, sizeof( help_rec_t ),
           compare_elem );

  return TRUE;	/*  Got help successfully  */
}


/*	COMPARE_ELEM -  Used by the quicksort library routine to classify
	one element as greater than or less than another.
*/

int  compare_elem  ( const void *elem_1, const void *elem_2 )
{
  return  ( strcmp ( ((help_rec_t *)elem_1) -> name, 
                     ((help_rec_t *)elem_2) -> name ) );
}


/*	HELP -  Search for help on a specific topic.  The topic is specified
	by the first parameter.  The second indicates the number of spaces to
	indent the help, while the last parameter states whether or not to
	put a small title field in the beginning of the help information.

	A help topic of NULL will return general help, if it is available.
	Likewise, if help is available, a TRUE value will be returned, FALSE
	otherwise.
*/

boolean  help  ( char *topic, int indent, boolean title )
{
  char *ind;	/*  Character indention string  */
  int  loc,	/*  Index of the help topic  */
       i;	/*  Indexing variable  */

  /*  Allocate memory for and fill indention string  */

  ind = (char *)alloc_mem ( indent + 1, sizeof( char ), "Help" );
  for  ( i = 0 ; i < indent ; i++ )
    ind [i] = ' ';
  ind [indent] = '\0';

  /*  Conduct search for the help topic specified  */

  if  ( topic == NULL )  {
    if  ( (loc = find_help( "HELP" )) == NOT_FOUND )  {
      printf  ( "%sSorry, General help is not available.\n", ind );
      return FALSE;
    }
  }  else  if  ( topic [0] == '?' )  {
    list_help  ( );
    return TRUE;
  }  else  {
    str_upper( topic );
    if  ( (loc = find_help( topic )) == NOT_FOUND )  {
      printf  ( "%sSorry, help is not available for %s\n", ind, topic );
      return FALSE;
    }
  }

  /*  Print title of the help topic  */

  if  ( title )  {
    printf  ( "%s%s\n", ind, help_data.topics [loc].name );
    printf  ( "%s", ind );
    for  ( i = 0 ; i < strlen( help_data.topics [loc].name ) ; i++ )
      printf ( "~" );
    printf  ( "\n" );
  }

  /*  Print the help information  */

  for  ( i = 0 ; i < help_data.topics [loc].Nlines ; i++ )
    printf  ( "%s%s", ind, help_data.topics [loc].descript [i] );

  free( ind );		/*  Deallocate indention and return affirmative res  */
  return TRUE; 
}


/*	FIND_HELP -  Does a linear search through the topics until the one
	specified is found.  Returns the index of the topic desired, or a
	NOT_FOUND value if the topic was not in the list.
*/

int  find_help  ( char *topic )
{
  int loc;	/*  Location of the topic being searched  */

  for  ( loc = 0 ; loc < help_data.Ntopics ; loc++ )
    if  ( !strcmp( topic, help_data.topics [loc].name ) )
      return loc;

  return NOT_FOUND;
}


/*	LIST_HELP -  List, in two columns, all topics for which help is
	available.
*/

void  list_help  ( void )
{
  int len,	/*  Length of the last help topic  */
      i,j;	/*  Indexing variable  */

  printf  ("Help is available on the following topics:\n");

  for  ( i = 0 ; i < help_data.Ntopics ; i++ )  {
    len = strlen( help_data.topics [i].name );
    printf  ( "  %s", help_data.topics [i].name );
    if  ( (i % 2) == 0 )
      for  ( j = len ; j < 39 ; j++ )
        printf (" ");
    else
      printf ("\n");
  }

  if  ( (help_data.Ntopics % 2) == 0 )
    printf  ("\n");
}

/*	CLOSE_HELP -  Deallocates the memory used by help in your program.  
	Help becomes unavailable, but memory is freed.
*/

void  close_help  ( void )
{
  int i, j;	/*  Indexing variables  */

  for  ( i = 0 ; i < help_data.Ntopics ; i++ )  {
    free( help_data.topics [i].name );
    for  ( j = 0 ; j < help_data.topics [i].Nlines ; j++ )
      free( help_data.topics [i].descript [j] );
    free ( help_data.topics [i].descript );
  }

  free( help_data.topics );
}
