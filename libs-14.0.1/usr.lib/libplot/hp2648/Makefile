#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1 (Berkeley) 5/7/85
#
#
CFLAGS=	-O -DNeXT_MOD -bsd
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o subr.o

../libplot2648:	${OBJS}
	ar cu ../libplot2648 ${OBJS}

${OBJS}:	hp2648.h

clean:
	rm -f ${OBJS} errs a.out core
