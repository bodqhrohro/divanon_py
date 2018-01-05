/*	Queue Library

	v1.0
	Matt White  (mwhite+@cmu.edu)
	4/2/93

	This library provides simple queue routines.  It accepts any data
	type, as long as size is known in advance.  Any questions should
	be directed to: mwhite+@cmu.edu.
*/


/*  Include Files  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"


/*	ENQUEUE -  Inserts a data item of size DATA_SIZE into the queue
	specified.
*/

void enqueue  ( queue *this_queue, void *new_data, int data_size )
{
  queue_node *new_node;		/*  Temporary pointer to the new node  */

  /*  Allocate memory for the new data  */

  if  ( (new_node = (queue_node *)malloc ( sizeof( queue_node ) )) == NULL )  {
    fprintf  ( stderr, "\n\nERROR: Unable to allocate memory in ENQUEUE\n\n" );
    exit( 1 );
  }
  if  ( (new_node -> data = malloc ( data_size )) == NULL )  {
    fprintf  ( stderr, "\n\nERROR: Unable to allocate memory in ENQUEUE\n\n" );
    exit( 1 );
  }

  /*  Insert information into the new node  */

  memcpy ( new_node -> data, new_data, data_size );
  new_node -> size = data_size;
  new_node -> next = NULL;

  /*  Link the new node into the queue  */

  if  ( QUEUE_ISEMPTY( *this_queue ) )  {
    this_queue -> tail = new_node;
    this_queue -> head = new_node;
  } else {
    this_queue -> tail -> next = new_node;
    this_queue -> tail = new_node;
  }

  (this_queue -> num_elem)++;	/*  Update the number of elements  */
}


/*	DEQUEUE -  This function removes the element from the front of the
	queue.  Note that the data is actually copied over into the void
	pointer passed to the function.  It is important that this pointer
	have sufficient memory allocated to it to hold the data passed back.
	If the queue is empty, this function returns a FALSE (0) value,
	otherwise a TRUE (1) value.
*/

int dequeue  ( queue *this_queue, void *data )
{
  queue_node *old_node;		/*  A pointer to the node being removed  */

  if  ( !QUEUE_ISEMPTY( *this_queue ) )  {
    
    /*  Copy the data over to the pointer  */

    memcpy  ( data, this_queue -> head -> data, this_queue -> head -> size );

    /*  Unlink the node from the rest  */

    old_node = this_queue -> head;
    this_queue -> head = this_queue -> head -> next;
    if  ( this_queue -> num_elem == 1 )
      this_queue -> tail = NULL;

    /*  Free the memory used by the node and it's data  */

    free ( old_node -> data );
    free ( old_node );

    (this_queue -> num_elem)--;		/*  Adjust the number of elements  */

    return TRUE;
  }
  return FALSE;
}


/*	DEQUEUEP -  This function is essentially the same as DEQUEUE, except
	that, instead of copying the data to a pointer, this function sets the
	pointer passed to it to the location of the data and then returns the
	number of bytes of that data item.  Note that, since the data is not
	actually copied, this can be significantly faster than a standard
	dequeue.  Also note that memory should NOT be allocated to the pointer
	passed to the function.
*/

int  dequeue_p  ( queue *this_queue, void **data )
{
  queue_node *old_node;
  int 	     data_size;

  if  ( !QUEUE_ISEMPTY( *this_queue ) )  {

    /*  Unlink the first node from the queue  */

    old_node = this_queue -> head;
    this_queue -> head = old_node -> next;
    if  ( this_queue -> num_elem == 1 )
      this_queue -> tail = NULL;

    /*  Copy the data from the old node  */

    *data = old_node -> data;
    data_size = old_node -> size;

    /*  Free the memory used by the old data node  */

    free( old_node );
    
    (this_queue -> num_elem)--;
    
    return data_size;
  }
  return 0;
}


/*	FLUSH_QUEUE -  This function discards the contents of the queue
	specified.
*/
      
void flush_queue  ( queue *this_queue )
{
  queue_node *temp;	/*  Temporary pointer to help delete nodes  */
 
  while  ( !QUEUE_ISEMPTY( *this_queue ) )  {

    /*  Unlink the head node  */

    temp = this_queue -> head;
    this_queue -> head = this_queue -> head -> next;

    /*  Free the memory used by the old node and it's data  */

    free ( temp -> data );
    free ( temp );
  }

  this_queue -> num_elem = 0;		/*  Reset counter and pointers  */
  this_queue -> tail     = NULL;
}




