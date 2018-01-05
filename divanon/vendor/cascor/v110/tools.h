/*	Extended C Toolbox Routines

	v1.0
	Matt White  (mwhite+@cmu.edu)
	6/23/93
*/

#ifndef TOOLS
#define TOOLS

/*	Data Type Definitions	*/

typedef int boolean;  /*  This is a simple boolean data enumeration.    */
		      /* Its value is always either 1 or 0.             */
#define TRUE  1
#define YES   1
#define ON    1
#define FALSE 0
#define NO    0
#define OFF   0


/*	Function Prototypes	*/

void     *alloc_mem	( int, int, char * );

char     *btoa		( boolean, int );
boolean  atob		( char * );

void    str_upper	( char * );

boolean is_int		( char * );
boolean is_float	( char * );
int	count_char	( char *, char, char * );

boolean file_exist	( char * );
boolean get_yn		( boolean );
void    add_ext		( char *, char *, char *, char **, int );

boolean init_help	( char * );
boolean help		( char *, int, boolean );
void    close_help	( void );

#endif
