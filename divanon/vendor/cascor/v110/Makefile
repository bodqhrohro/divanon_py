#	Cascade Correlation Algorithm
#	Makefile
#
#	v1.0
#	Matt White  (mwhite+@cmu.edu)
#	7/6/93
#
#	Replace CC with whatever compiler gives you the best results.
#	Likewise, replace CFLAGS with whatever compiler directives give you the
#	best optimization.  The compiler used should be ANSI/ISO compatible.

CC = cc
LDLIBS = -lm
CFLAGS =

CASCOROBJS =	cascor.o interface.o parse.o tools.o queue.o

cascor:		$(CASCOROBJS)
		$(CC) $(CFLAGS) -o cascor $(CASCOROBJS) $(LDLIBS)

cascor.o:	cascor.h parse.h interface.h tools.h
interface.o:	parse.h tools.h cascor.h interface.h
parse.o:	parse.h queue.h tools.h
tools.o:	tools.h queue.h
queue.o:	queue.h

clean:
		"rm" -f -e core *.o *~ *.u

