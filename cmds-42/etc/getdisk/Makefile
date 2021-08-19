#
# Generic Makefile for simple commands
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#
PROGRAM= getdisk
BINDIR=	/usr/etc
DSTDIRS= $(DSTROOT)$(BINDIR)

CDEFS=	-DNeXT_MOD
CFLAGS=	$(CDEFS) -fwritable-strings
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= getdisk.c
SFILES=

PRINTFILES = Makefile $(HFILES) $(YFILES) $(CFILES) $(SFILES)

GARBAGE= errs Make.out

STD_COPTS= -O
PROF_COPTS= -pg -O
DBG_COPTS= -g -DDEBUG

STD_LDOPTS= -object
PROF_LDOPTS= -object
DBG_LDOPTS=

STD_NAME=
PROF_NAME=_PROF
DBG_NAME=_DBG

SRCS= $(HFILES) $(MIGHFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME=${$(TYPE)_NAME}

VPATH=..

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	md -d -u Makedep $*.d

all:	STD

STD PROF DBG:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(PROGRAM)

install: DSTROOT $(DSTDIRS) STD ALWAYS
	install $(IFLAGS) STD_OBJS/$(PROGRAM) $(DSTROOT)$(BINDIR)/$(PROGRAM)

#
# Building the command
#
$(PROGRAM): $(OBJS) vers.o
	$(CC) $(LDFLAGS) $(LDOPTS) -o $@ $(OBJS) vers.o $(LIBS)
	
vers.c: $(OBJS)
	@rm -f $@
	vers_string -c $(PROGRAM)$(NAME) > $@

#
# Standard targets
#

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags vers.c Makedep $(GARBAGE)
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
