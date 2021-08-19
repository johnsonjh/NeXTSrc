#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= cs
BUILDS= STD
INSTALLS= STD.install
LIBDIR=	$(DSTROOT)/usr/lib
DSTDIRS= $(LIBDIR)

CDEFS=	-DNeXT_MOD
CFLAGS=	$(CDEFS) -fwritable-strings
IFLAGS= -r -m 444
LIBS=

HFILES=
YFILES= parsedate.y
CFILES= abspath.c atoh.c atoo.c atot.c boolarg.c bsearch.c chrarg.c ci.c \
	cmuttyname.c del.c dfork.c doublearg.c editor.c errmsg.c expand.c \
	fdate.c ffilecopy.c fgetpass.c filecopy.c floatarg.c fold.c \
	fwantread.c fwantwrite.c getbool.c getchr.c getdouble.c getfloat.c \
	gethex.c gethostattr.c getint.c getlong.c getname.c getoct.c \
	getpwwho.c getsearch.c getshort.c getstab.c getstr.c gtime.c \
	hexarg.c intarg.c longarg.c movefile.c nxtarg.c octarg.c openp.c \
	path.c prstab.c putenv.c quit.c run.c salloc.c searcharg.c searchp.c \
	setpath.c shortarg.c sindex.c skipto.c srchscore.c stabarg.c stablk.c \
	stabsearch.c stlmatch.c strarg.c tenex.c ulstrcmp.c ulstrncmp.c \
	vmnlist.c wantread.c wantwrite.c
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
	rm -f $@
	$(AR) cr $@ `lorder $(OBJS) $(LIBRARY)_vers.o | tsort`

$(LIBRARY)_vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -l $(LIBRARY) > $@

parsedate.c: ../parsedate.y
	yacc ../parsedate.y
	${MV} y.tab.c parsedate.c
	ed - parsedate.c <../parsedate.scr

#
# Standard targets
#

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags *_vers.c Makedep $(DERIVED) $(GARBAGE)
	-rm -f DBG_OBJS/* PROF_OBJS/* STD_OBJS/*

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
