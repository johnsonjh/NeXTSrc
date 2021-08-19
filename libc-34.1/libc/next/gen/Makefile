#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= genlib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../.. -I../../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	gmon.h
YFILES=
CFILES= atof.c disktab.c ecvt.c isinf.c isnan.c setjmperr.c	\
	getmachhead.c getsecbyname.c getsegbyname.c get_end.c	\
	gmon.c modf.c fabs.c ldexp.c

SFILES=	_setjmp.s abs.s alloca.s bcmp.s bcopy.s bzero.s 	\
	ffs.s index.s insque.s remque.s rindex.s setjmp.s 	\
	strcat.s strcmp.s strcpy.s strlen.s strncat.s 		\
	strncmp.s strncpy.s _dbltopdfp.s _pdfptodbl.s

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

.s.o:
	@rm -f x.s
	$(CC) -MD -E $(CFLAGS) $(COPTS) -DASSEMBLER $< >x.s
	$(AS) x.s -o $@
	@rm -f x.s

gmon.o: gmon.h
	@rm -f x.s
	@if [ "$(TYPE)" = "PROF" ];					\
	then 								\
		echo "gmon.c - PROFILED";				\
		cc -MD -O -S $(CFLAGS) -O ../gmon.c;			\
	else								\
		echo "gmon.c - NOT PROFILED";				\
		cc -MD -O -S $(CFLAGS) $(COPTS) ../gmon.c;		\
	fi
	ex - gmon.s < ../gmon.ex
	as -o gmon.o gmon.s
	ld -X -r -o x.o gmon.o
	${MV} x.o gmon.o
	rm -f x.o gmon.s
	md -d -u Makedep gmon.d

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
