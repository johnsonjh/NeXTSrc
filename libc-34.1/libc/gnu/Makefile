#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= gnulib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	gnulib.h
YFILES=
CFILES=	adddi3.c subdi3.c muldi3.c divdi3.c moddi3.c udivdi3.c	\
	umoddi3.c negdi2.c anddi3.c iordi3.c xordi3.c lshrdi3.c	\
	lshldi3.c ashldi3.c ashrdi3.c one_cmpldi2.c bdiv.c	\
	cmpdi2.c ucmpdi2.c fixunsdfdi.c fixdfdi.c floatdidf.c	\
	fixunsdfsi.c eprintf.c builtin_New.c builtin_del.c	\
	fixdfsi.c
SFILES=

DERIVED=
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

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

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

-include $(DIR)/Makedep
