#
# Generic Makefile for simple commands
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#
# Who and where
#
PROGRAM= diff3
BINDIR=	/usr/lib
SHBINDIR= /usr/bin
DSTDIRS= $(DSTROOT)$(BINDIR) $(DSTROOT)$(SHBINDIR)

#
# Source files
#
HFILES=
YFILES=
CFILES= $(PROGRAM).c
SFILES=

PRINTFILES = Makefile $(HFILES) $(YFILES) $(CFILES) $(SFILES)
TABSIZE=8

GARBAGE= errs Make.out

#
# Global options that apply to all types of builds
#
CFLAGS= -fwritable-strings
LDFLAGS= -object
LIBS= -lsys_s
IFLAGS= -s

#
# Options for release builds
#
STD_COPTS= -O
STD_LDOPTS=
STD_NAME=

#
# Options for profiling builds
#
PROF_COPTS= -pg -O
PROF_LDOPTS= -pg
PROF_NAME=_PROF

#
# Options for debugging builds
#
DBG_COPTS= -g -DDEBUG
DBG_LDOPTS=
DBG_NAME=_DBG

#
# Default target
#
all:	STD

#
# Extra install operations go here
#
install_extra:
	install -c -xs -m 555 diff3.sh $(DSTROOT)$(SHBINDIR)/diff3

#
# Boiler plate
# Hopefully, the rest doesn't have to be modified
#
SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

STD_VPATH=..
PROF_VPATH=..
DBG_VPATH=..

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME=${$(TYPE)_NAME}
VPATH=${$(TYPE)_VPATH}

#
# CCFLAGS are for command line use
# e.g. make CCFLAGS="-I/foo/bar/include"
#
.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) $(CCFLAGS) -c -o $@ $<
	md -d -u Makedep $*.d

#
# Standard targets
#

STD PROF DBG:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(PROGRAM)

install: DSTROOT $(DSTDIRS) STD ALWAYS install_extra
	install $(IFLAGS) STD_OBJS/$(PROGRAM) $(DSTROOT)$(BINDIR)/$(PROGRAM)

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags vers.c Makedep $(GARBAGE)
	-rm -rf DBG_OBJS PROF_OBJS STD_OBJS

tags:	ALWAYS
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
$(PROGRAM): $(OBJS) vers.o
	$(CC) $(LDFLAGS) $(LDOPTS) -o $@ $(OBJS) vers.o $(LIBS)
	
vers.c: $(OBJS)
	@rm -f $@
	vers_string -c $(PROGRAM)$(NAME) > $@

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
