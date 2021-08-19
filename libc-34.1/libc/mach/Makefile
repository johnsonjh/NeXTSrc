#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= machlib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I.. -I../../../usr.include
INCLUDESRC=../../../usr.include
IFLAGS= -r -m 444
LIBS=

DEFS=	exc.defs mach.defs mach_host_u.defs
HFILES=	
#
# These header files reside in usr/include/servers but are
# referenced via include " " so that they must be copied here
# for this to build properly
SERVERHCOPY= netname.h bootstrap.h
#
# Servers provided in the library itself
SERVERCFILES= exc_server.c
#
# Interfaces to servers running seperately
# ipcx_user.c is left out for now
USERSERVERCFILES= netname_u.c bootstrap_u.c
USERSERVEROFILES=$(USERSERVERCFILES:.c=.o)

#
# Interfaces to servers provided in the library or the kernel
USERCFILES=	exc_u.c mach_u.c mach_host_u.c
YFILES=
CFILES=	$(USERCFILES) $(SERVERCFILES) $(USERSERVERCFILES)	\
	mach_error.c mach_init.c mach_data.c exception.c	\
	exc_catcher.c msg.c sbrk.c slot_name.c ts_convert.c	\
	compat.c
SFILES= mach_traps.s

DERIVED= $(USERCFILES) $(SERVERCFILES) $(USERSERVERCFILES)
GARBAGE= 

SHLIB_NAME=_s
PROF_NAME=_p
STD_NAME=

SHLIB_COPTS= -O -g -i$(SHLIBIDIR)/shlib.h -DSHLIB
PROF_COPTS= -O -pg
STD_COPTS= -O -g

SHLIB_VPATH=..
PROF_VPATH=..
STD_VPATH=..

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME= ${$(TYPE)_NAME}
VPATH= ${$(TYPE)_VPATH}
LIBRARY= $(LIBNAME)${$(TYPE)_NAME}

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<

.s.o:
	@rm -f x.s
	$(CC) -MD -E $(CFLAGS) $(COPTS) -DASSEMBLER $< >x.s
	$(AS) x.s -o $@
	@rm -f x.s

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

$(USERCFILES): ALWAYS
	(CWD=`pwd`;cd $(INCLUDESRC); \
	 make interface TOFILE=$$CWD/$@ FROMFILE=$@ )

$(USERSERVEROFILES): $(SERVERHCOPY)

$(SERVERHCOPY) $(USERSERVERCFILES): ALWAYS
	(CWD=`pwd`;cd $(INCLUDESRC); \
	 make interface TOFILE=$$CWD/$@ FROMFILE=servers/$@ )

$(SERVERCFILES): ALWAYS
	(CWD=`pwd`;cd $(INCLUDESRC); \
	 make interface TOFILE=$$CWD/$@ FROMFILE=$@ )


#
# Building the library
#
$(LIBRARY): $(OBJS) depend
	$(AR) cr $@ `lorder $(OBJS) | tsort`

#
# Standard targets
#
depend:
	md -d -u Makedep *.d

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags Makedep $(DERIVED) $(GARBAGE)
	-rm -f SHLIB_OBJS/* PROF_OBJS/* STD_OBJS/*

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS) SHLIB_OBJS PROF_OBJS STD_OBJS:
	mkdirs $@

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include $(DIR)Makedep


