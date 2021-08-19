#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= curses
BUILDS= STD PROF
INSTALLS= STD.install PROF.install
LIBDIR=	$(DSTROOT)/usr/lib
INCDIR= $(DSTROOT)/usr/include
DSTDIRS= $(LIBDIR) $(INCDIR)

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	$(CDEFS) -fwritable-strings
IFLAGS= -r -m 444
LIBS=

HFILES= curses.h
YFILES=
CFILES= addch.c addstr.c box.c clear.c clrtobot.c \
	clrtoeol.c cr_put.c cr_tty.c curses.c delch.c \
	deleteln.c delwin.c endwin.c erase.c fullname.c \
	getch.c getstr.c id_subwins.c idlok.c initscr.c \
	insch.c insertln.c longname.c move.c mvprintw.c \
	mvscanw.c mvwin.c newwin.c overlay.c overwrite.c \
	printw.c putchar.c refresh.c scanw.c scroll.c \
	standout.c toucholap.c touchwin.c tstp.c \
	unctrl.c
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
	install curses.h $(INCDIR)

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
