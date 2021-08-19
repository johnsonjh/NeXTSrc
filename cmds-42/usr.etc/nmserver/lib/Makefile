#
# Mach Operating System
# Copyright (c) 1987 Carnegie-Mellon University
# All rights reserved.  The CMU software License Agreement specifies
# the terms and conditions for use and redistribution.
#

include ${MACHINE}/OBJECTS

VPATH=		${$(MACHINE)VPATH}:${STDVPATH}

CFLAGS=		-O -bsd
CPP=		/lib/cpp
CC=		cc
KERNELDIR=	/usr/mk

INCL=	-I. -I${MACHINE} -I${KERNELDIR} -I${KERNELDIR}/sys

STDOBJS=	vmtplib.o


.c.o:
	${CPP} ${INCL} -E $*.c >$*.cpp.s
	${AS} -o $*.o $*.cpp.s
	-ld -x -r $*.o
	mv a.out $*.o
	rm -f $*.cpp.s

.s.o:
	${CC} -c $*.s

libvmtp.a: ${STDOBJS} ${$(MACHINE)OBJS}
	@echo "building normal libvmtp.a"
	@ar cru libvmtp.a $?

vmtplib.o:	vmtplib.c
	${CC} ${CFLAGS} ${INCL} -c vmtplib.c

clean:
	rm -f *.o errs a.out core libvmtp.a
