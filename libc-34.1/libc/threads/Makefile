#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= threadslib
MACHINE = next
IMPL = MTHREAD
BUILDS= STD PROF

INLINE = ../cthread_inline
MV = /bin/mv -f
AS = /bin/as
RM = /bin/rm -f

CDEFS=	-D$(IMPL) -I. -I..
CFLAGS=	-S $(CDEFS) -I../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	
YFILES=
CFILES=	cprocs.c cthreads.c threads_data.c mig_support.c		\
	stack.c cthreads_sync.c cthreads_time.c sched.c			\
	ipc.c unix.c spin.c thread.c
SFILES= csw.s lock.s

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
	$(CC) -MD $(CFLAGS) $(COPTS) $<
	$(MV) $*.s $*.S
	$(INLINE) $*.S > $*.s
	$(AS) -o $*.o $*.s
	$(RM) $*.s $*.S

.s.o:
	@rm -f x.s
	${CC} -MD -E $(COPTS) -DASSEMBLER $< > x.s
	$(MV) x.s x.S
	$(INLINE) x.S > x.s
	${AS} x.s -o $@

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

spin.c:		$(MACHINE)/spin.c
	$(CP) $(MACHINE)/spin.c	.

thread.c:	$(MACHINE)/thread.c
	$(CP) $(MACHINE)/thread.c .

lock.s:		$(MACHINE)/lock.s
	$(CP) $(MACHINE)/lock.s	.

csw.s:		$(MACHINE)/null_csw.s
	$(CP) $(MACHINE)/null_csw.s csw.s

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
