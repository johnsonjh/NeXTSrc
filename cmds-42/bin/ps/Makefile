#
# Generic Makefile for simple commands
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#
# Who and where
#
PROGRAM= ps
BINDIR=	/bin
DSTDIRS= $(DSTROOT)$(BINDIR)

#
# What
#
HFILES=
YFILES=
CFILES= $(PROGRAM).c
SFILES=

PRINTFILES = Makefile $(HFILES) $(YFILES) $(CFILES) $(SFILES)
TABSIZE=8

GARBAGE= errs Make.out

#
# How
#
# Global options that apply to all types of builds
#
CFLAGS=
IFLAGS= -s -m 4755
LIBS= -ltermcap -lsys_s

#
# Options for release builds
#
STD_COPTS= -O
STD_LDOPTS= -object
STD_NAME=

#
# Options for profiling builds
#
PROF_COPTS= -pg -O
PROF_LDOPTS= -object
PROF_NAME=_PROF

#
# Options for debugging builds
#
DBG_COPTS= -g -DDEBUG
DBG_LDOPTS= -object
DBG_NAME=_DBG

#
# Boiler plate
#
SRCS= $(HFILES) $(MIGHFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

STD_VPATH=..
PROF_VPATH=..
DBG_VPATH=..

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME=${$(TYPE)_NAME}
VPATH=${$(TYPE)_VPATH}

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	md -d -u Makedep $*.d

#
# Standard targets
#
all:	STD

STD PROF DBG:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(PROGRAM)

install: DSTROOT $(DSTDIRS) STD ALWAYS
	install $(IFLAGS) STD_OBJS/$(PROGRAM) $(DSTROOT)$(BINDIR)/$(PROGRAM)

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags Makedep $(GARBAGE)
	-rm -rf DBG_OBJS PROF_OBJS STD_OBJS

tags:	$(MIGFILES) ALWAYS
	etags $(SRCS)
	ctags $(SRCS)
	
print:
	for i in $(PRINTFILES); \
	do \
		expand -$(TABSIZE) $$i >/tmp/$$i; \
	done; \
	cd /tmp; enscript $(PRINTFILES); rm $(PRINTFILES)
	
vgrind:
	touch index
	vgrind $(HFILES) $(CFILES)
	vgrind -x

#
# Building the command
#
$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) $(LDOPTS) -o $@ $(OBJS) $(LIBS)
	
#
# Internal targets
#
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

ALWAYS:

-include $(DIR)/Makedep
