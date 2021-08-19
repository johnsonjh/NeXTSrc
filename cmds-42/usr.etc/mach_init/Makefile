#
# Generic Makefile for MIG-based servers
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#
PROGRAM= mach_init
BINDIR=	usr/etc
LINKDIR= private/etc
INCDIR= /usr/include
DSTDIRS= $(DSTROOT)/usr/etc $(DSTROOT)/private/etc

CFLAGS=	$(CDEFS) -Wall
IFLAGS= -s
#
# Do not link this command with the libsys shlib
# Performance modification.  Shlib this and see what happens.
#	Morris Meyer
#
LIBS= -lsys_s

DEFS=	$(INCDIR)/servers/bootstrap.defs

HFILES= bootstrap_internal.h error_log.h lists.h parser.h
YFILES=
CFILES= bootstrap.c error_log.c lists.c parser.c rpc_services.c
SFILES=

PRINTFILES = Makefile $(DEFS) $(HFILES) $(YFILES) $(CFILES) $(SFILES)

MIGCFILES= bootstrapServer.c bootstrapUser.c
MIGHFILES= bootstrap.h
MIGFILES= $(MIGCFILES) $(MIGHFILES)
GARBAGE= index

STD_COPTS= -g -O
PROF_COPTS= -pg -O
DBG_COPTS= -g -DDEBUG

STD_LDOPTS=
PROF_LDOPTS= -pg
DBG_LDOPTS=

STD_NAME=
PROF_NAME=_PROF
DBG_NAME=_DBG

SRCS= $(HFILES) $(MIGHFILES) $(YFILES) $(CFILES) $(MIGCFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o) $(MIGCFILES:.c=.o)

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME=${$(TYPE)_NAME}

VPATH=..

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	md -d -u Makedep $*.d

all:	STD

STD PROF DBG:	$(MIGFILES) $$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(PROGRAM)

install: DSTROOT $(DSTDIRS) STD ALWAYS
	install $(IFLAGS) STD_OBJS/$(PROGRAM) $(DSTROOT)/$(BINDIR)/$(PROGRAM)
	rm -f ${DSTROOT}/${LINKDIR}/${PROGRAM}
	ln -s ../../${BINDIR}/$(PROGRAM) ${DSTROOT}/${LINKDIR}

#
# Building the command
#
$(PROGRAM): $(OBJS) vers.o
	$(CC) $(LDFLAGS) $(LDOPTS) -o $@ $(OBJS) vers.o $(LIBS)
	
$(MIGFILES): $(DEFS)
	mig $(DEFS)

vers.c: $(OBJS)
	@rm -f $@
	vers_string -c $(PROGRAM)$(NAME) > $@

#
# Standard targets
#

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags vers.c Makedep $(MIGFILES) $(GARBAGE)
	-rm -rf DBG_OBJS PROF_OBJS STD_OBJS

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS) DBG_OBJS PROF_OBJS STD_OBJS:
	mkdirs $@

tags:	$(MIGFILES) ALWAYS
	etags $(SRCS)
	ctags $(SRCS)
	
print:
	for i in $(PRINTFILES); \
	do \
		expand -4 $$i >/tmp/$$i; \
	done; \
	cd /tmp; enscript $(PRINTFILES); rm $(PRINTFILES)
	
vgrind:
	touch index
	vgrind $(HFILES) $(CFILES)
	vgrind -x

ALWAYS:

-include $(DIR)/Makedep


