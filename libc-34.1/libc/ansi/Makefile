#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= ansilib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	
YFILES=
CFILES=	atexit.c bsearch.c clock.c difftime.c div.c feof.c 	\
	ferror.c fgetpos.c fsetpos.c getc.c isalnum.c isalpha.c \
	iscntrl.c isdigit.c isgraph.c islower.c isprint.c 	\
	ispunct.c isspace.c isupper.c isxdigit.c ldiv.c 	\
	localeconv.c mblen.c mbstowcs.c mbtowc.c putc.c		\
	raise.c setlocale.c setvbuf.c strcoll.c strerror.c	\
	strftime.c strstr.c strtol.c tmpfile.c tmpnam.c		\
	tolower.c toupper.c va_end.c wcstombs.c wctomb.c
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
