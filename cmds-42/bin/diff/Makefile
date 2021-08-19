#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= diff diffh
BINDIR=	$(DSTROOT)/bin
LIBDIR=	$(DSTROOT)/usr/lib
DSTDIRS= $(BINDIR) $(LIBDIR)

CDEFS=	-DDIFF='"/bin/diff"' \
	-DDIFFH='"/usr/lib/diffh"' -DPR='"/bin/pr"'
CFLAGS=	-O -g $(CDEFS) -fwritable-strings
IFLAGS= -s
LIBS= -lsys_s

HFILES=	diff.h
CFILES= diff.c diffdir.c diffh.c diffreg.c
YFILES=
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)

.c.o:
	$(CC) -MD $(CFLAGS) $(COPTS) -c $<
	md -d -u Makedep $*.d

all:	$(PRODUCTS)

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} diff.NEW $(BINDIR)/diff
	install ${IFLAGS} diffh.NEW $(LIBDIR)/diffh

#
# Building diff
#
DIFF_OBJS= diff.o diffdir.o diffreg.o

diff.NEW:	${DIFF_OBJS} diff_vers.o
	$(CC) $(CFLAGS) -o $@ ${DIFF_OBJS} diff_vers.o ${LIBS}

diff_vers.c: ${DOVERS} ${DIFF_OBJS}
	@rm -f $@
	vers_string -c diff > $@

#
# Building diffh
#
DIFFH_OBJS= diffh.o

diffh.NEW:	${DIFFH_OBJS} diffh_vers.o
	$(CC) $(CFLAGS) -o $@ ${DIFFH_OBJS} diffh_vers.o ${LIBS}

diffh_vers.c: ${DOVERS} ${DIFFH_OBJS}
	@rm -f $@
	vers_string -c diffh > $@

#
# Standard targets
#
$(PRODUCTS):	$$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCTS) $(DERIVED) \
		$(GARBAGE)

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS):
	mkdirs $@

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
