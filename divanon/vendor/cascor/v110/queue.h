/*	Queue Library

	v1.0
	Matt White (mwhite+@cmu.edu)
	4/2/93

	This library supplies relatively general and simple to use queue
	routines.  Any questions concerning this library should be directed
	to:  mwhite+@cmu.edu.
*/

#ifndef QUEUE
#define QUEUE

/*	Structure Definintions	*/

typedef struct queue_node_type  {	/*  This structure holds an element  */
  void                   *data;		/* of the queue.  It includes a ptr  */
  int                    size;		/* to the actual data, size of the   */
  struct queue_node_type *next;		/* data, and a ptr to the next node. */
} queue_node;

typedef struct {
  queue_node *head,	/*  Pointer to the first element of the queue  */
             *tail;	/*  Pointer to the last element of the queue  */
  int	     num_elem;	/*  Number of elements in the queue  */
} queue;


/*  Constant Declarations  */
  
#define TRUE  1
#define FALSE 0


/*  Macro Declarations  */

#define QUEUE_INIT(x)    { ((x).head) = ((x).tail) = NULL; ((x).num_elem) = 0;}
#define QUEUE_ISEMPTY(x) ( ((x).head) == NULL )


/*  Function Prototypes  */

void  enqueue		( queue *, void *, int );
int   dequeue		( queue *, void * );
int   dequeue_p		( queue *, void ** );
void  flush_queue	( queue * );

#endif





