#
# Generic Makefile for multiple command builds
# (c) NeXT, Inc.  1989, 1990.  All rights reserved.
#
PROGRAMS= fbshow mkfont
INSTALLS= fbshow.install
FONTDIR= /usr/lib/bootimages
FONTS= Helvetica.18 Helvetica-Bold.18 Helvetica-Oblique.18 \
	Helvetica-BoldOblique.18
DSTDIRS= $(DSTROOT)$(fbshow_DIR) $(DSTROOT)$(FONTDIR)

HFILES= font.h
YFILES=
CFILES= fbshow.c mkfont.c
SFILES=

PRINTFILES = Makefile $(HFILES) $(YFILES) $(CFILES) $(SFILES)

GARBAGE= errs Make.out Helvetica.18 Helvetica-Bold.18 Helvetica-Oblique.18 \
	Helvetica-BoldOblique.18

fbshow_OBJS= fbshow.o
fbshow_LDOPTS= -object
fbshow_LIBS= -lsys_s
fbshow_IFLAGS= -s
fbshow_DIR= /usr/etc

mkfont_OBJS= mkfont.o
#mkfont_LDOPTS= -object	// wait until can be used everywhere
mkfont_LDOPTS= 
mkfont_LIBS= -lsys_s
mkfont_IFLAGS= -s
mkfont_DIR= /usr/etc

CFLAGS=

STD_COPTS= -O
PROF_COPTS= -pg -O
DBG_COPTS= -g -DDEBUG

STD_LDOPTS=
PROF_LDOPTS=
DBG_LDOPTS=

STD_NAME=
PROF_NAME=_PROF
DBG_NAME=_DBG

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)

COPTS= ${$(TYPE)_COPTS}
LDOPTS= ${$(TYPE)_LDOPTS}
DIR= $(TYPE)_OBJS
NAME=${$(TYPE)_NAME}

VPATH=..

all:	STD fonts

fonts:	STD $(FONTS)

Helvetica.18: 18.Helve
	STD_OBJS/mkfont $? $@

Helvetica-Bold.18: 18.HelveBol
	STD_OBJS/mkfont $? $@

Helvetica-Oblique.18: 18.HelveObl
	STD_OBJS/mkfont $? $@

Helvetica-BoldOblique.18: 18.HelveBolObl
	STD_OBJS/mkfont $? $@

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<
	md -d -u Makedep $*.d

STD PROF DBG:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(PROGRAMS)

$(PROGRAMS): $${$$@_OBJS}
	@rm -f $@_vers.c
	vers_string -c $(PROGRAM)$(NAME) > $@_vers.c
	$(CC) $(CFLAGS) $(COPTS) -c -o $@_vers.o $@_vers.c
	$(CC) $(LDFLAGS) $(LDOPTS) $($@_LDOPTS) -o $@ $($@_OBJS) $@_vers.o \
	    $($@_LIBS) $(LIBS)

install: DSTROOT all $(DSTDIRS)
	CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD; \
	    cd STD_OBJS; \
		$(MAKE) -f ../Makefile DSTROOT=$$DSTROOT TYPE=STD $(INSTALLS)
	for i in $(FONTS); \
	do \
		install -m 444 $$i $(DSTROOT)$(FONTDIR); \
	done
	

$(INSTALLS): DSTROOT $${@:.install=} ALWAYS
	install ${$(@:.install=)_IFLAGS} $(@:.install=) $(DSTROOT)${$(@:.install=)_DIR}

#
# Standard targets
#

clean:	ALWAYS
	-rm -f *.o *.a TAGS tags *_vers.c Makedep $(GARBAGE)
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

ALWAYS depend:

-include $(DIR)/Makedep
