#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= y
BUILDS= STD PROF
#BUILDS= STD PROF DBG
INSTALLS= STD.install PROF.install
#INSTALLS= STD.install PROF.install DBG.install
LIBDIR=	$(DSTROOT)/usr/lib
DSTDIRS= $(LIBDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	$(CDEFS) -fwritable-strings
IFLAGS= -r -m 444
LIBS=

HFILES=
YFILES=
CFILES= libmai.c libzer.c
SFILES=

DERIVED=
GARBAGE=

STD_NAME=
PROF_NAME=_p
DBG_NAME=_g

STD_COPTS= -O
PROF_COPTS= -pg -O
DBG_COPTS= -gg

STD_LDOPTS= -x
PROF_LDOPTS= -X
DBG_LDOPTS= -X

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

VPATH=..
DIR=.

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	$(LD) $(LDFLAGS) $(LDOPTS) -r $@ -o $@.out
	rm -f $@
	mv $@.out $@
	md -d -u Makedep $*.d

all:	$(BUILDS)

STD PROF DBG:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile "COPTS=$($@_COPTS)" \
	    "LDOPTS=$($@_LDOPTS)" LIBRARY=$(LIBNAME)$($@_NAME) \
	    "DIR=$@_OBJS" lib$(LIBNAME)$($@_NAME).a

install: DSTROOT $(DSTDIRS) $(INSTALLS)

$(INSTALLS): DSTROOT $$(@:.install=)_OBJS ALWAYS
	CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD; \
	    cd $(@:.install=)_OBJS; \
		$(MAKE) -f ../Makefile DSTROOT=$$DSTROOT \
		    "COPTS=$(${@:.install=}_COPTS)" \
		    "LDOPTS=$(${@:.install=}_LDOPTS)" \
		    LIBRARY=$(LIBNAME)$(${@:.install=}_NAME) \
		    "DIR=$(@:.install=)_OBJS" $(@:.install=)install 

STDinstall PROFinstall DBGinstall: DSTROOT $(DSTDIRS) lib$(LIBRARY).a ALWAYS
	install $(IFLAGS) lib$(LIBRARY).a $(LIBDIR)

#
# Building the library
#
lib$(LIBRARY).a: $(OBJS) $(LIBRARY)_vers.o
	$(AR) cr $@ `lorder $(OBJS) $(LIBRARY)_vers.o | tsort`

$(LIBRARY)_vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -l $(LIBRARY) > $@

#
# Standard targets
#

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags *_vers.c Makedep $(DERIVED) $(GARBAGE)
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

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS depend:

-include $(DIR)/Makedep
