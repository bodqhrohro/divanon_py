/*	Cascade Correlation Learning Algorithm

	Unix Interface Code
	v1.1
	Matt White  (mwhite+@cmu.edu)
	5/31/94
*/


#ifndef INTERFACE
#define INTERFACE

/*	Include Files	*/

#include <time.h>
#include "cascor.h"


/*	Function Prototypes	*/

void	prog_id			( void );
void	list_parms		( void );
void	output_begin_trial	( int, time_t * );
void	output_train_results	( status_t );
void	output_val_results	( error_data );
void	output_cand_results	( status_t );
void	output_trial_results	( status_t, int, time_t );
void	output_run_results	( void );

void	exec_command_line	( int, char ** );

void	change_parms		( boolean );

void	save_weights		( char *, boolean, int, time_t );
void	load_weights		( char * );

void	check_interrupt		( void );
void	trap_ctrl_c		( int );

#endif
