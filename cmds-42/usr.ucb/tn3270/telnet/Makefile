#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	4.1 (Berkeley) 12/4/88
#

# The following is the telnet makefile for tn3270, using the shared telnet
# sources.

VPATH	=	
INCLUDES=	-I.
DEFINES	=	-DTN3270 -DNeXT_MOD -DTERMCAP
OPTIMIZE=	-O
DEBUG_FLAGS=	-g
CFLAGS	=	${OPTIMIZE} ${DEBUG_FLAGS} ${INCLUDES} ${DEFINES}
XCFLAGS=	${XINCLUDES} ${XDEFINES}
LD	=	ld
LDFLAGS	=	-r
PRINT	= print
ACTION = sccs tell
LIBC=
ALLH=	defines.h externs.h fdset.h general.h ring.h types.h
SRCS=	commands.c main.c network.c ring.c \
	sys_bsd.c telnet.c terminal.c \
	tn3270.c utilities.c
ALLHC=		${ALLH} ${SRCS}
ALLPRINT =	${ALLHC}
ALLSOURCE=	${ALLHC} Makefile Makefile_ultrix
OBJS=	commands.o main.o network.o ring.o sys_bsd.o \
	telnet.o terminal.o tn3270.o utilities.o

.c.o:
	${CC} -c ${CFLAGS} ${XCFLAGS} $<

all:	${OBJS} 

clean: FRC
	rm -f telprog.o ${OBJS} core telnet

depend: FRC ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

lint: FRC ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: FRC ${ALLHC}
	ctags ${ALLHC}

print:	FRC ${ALLPRINT}
	${PRINT} ${ALLPRINT}

action: FRC
	${ACTION}

clist:	FRC ${SRCS}
	@for i in ${SRCS} ; \
		do (echo ${DIRPATH}$$i); done

hclist:	FRC ${ALLHC}
	@for i in ${ALLHC} ; \
		do (echo ${DIRPATH}$$i); done

sourcelist:	FRC ${ALLSOURCE}
	@for i in ${ALLSOURCE} ../../telnet/Makefile ; \
		do (echo ${DIRPATH}$$i); done

FRC:


