#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
#PRODUCTS= zic zdump
PRODUCTS= zic zdump
BINDIR=	$(DSTROOT)/usr/etc
CONFDIR= ${DSTROOT}/etc/zoneinfo
DSTDIRS= $(BINDIR) $(CONFDIR)

# If you want something other than Pacific United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -l rightzone
LOCALTIME=	US/Pacific

# If you want code inspired by certain emerging standards, add
#	-DSTD_INSPIRED
# to the end of the "CFLAGS=" line.
#
# If you want to handle solar-time-based time zones, remove the
# "#define NOSOLAR" from the include file usr/include/tzfile.h.
# (and add solar87 to the DATA= line below).
CDEFS=	-DNeXT_MOD -D__STRICT_BSD__
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES=
CFILES= ialloc.c scheck.c zdump.c zic.c
SFILES=

DOCS=		Theory README Makefile newctime.3 tzfile.5 zic.8 zdump.8

DATA=		asia australasia europe etcetera northamerica \
		pacificnew systemv

DERIVED=
GARBAGE= *_vers.c

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install:	DSTROOT ${DSTDIRS} $(PRODUCTS) $(DATA)
		./zic.NEW -d ${CONFDIR} -l $(LOCALTIME) $(DATA)
		install $(IFLAGS) zic.NEW $(BINDIR)/zic
#		install $(IFLAGS) zdump.NEW $(BINDIR)/zdump

#
# Building zic
#
ZIC_OBJS=	zic.o scheck.o ialloc.o

zic.NEW:	$(ZIC_OBJS) zic_vers.o
	$(CC) $(CFLAGS) -o $@ $(ZIC_OBJS) zic_vers.o $(LIBS)

zic_vers.c: $(DOVERS) $(ZIC_OBJS)
	@rm -f $@
	vers_string -c zic > $@

#
# Building zdump
#
ZDUMP_OBJS=	zdump.o ialloc.o

zdump.NEW:	$(ZDUMP_OBJS) zdump_vers.o
	$(CC) $(CFLAGS) -o $@ $(ZDUMP_OBJS) zdump_vers.o $(LIBS)

zdump_vers.c: $(DOVERS) $(ZDUMP_OBJS)
	@rm -f $@
	vers_string -c zdump > $@

#
# Standard targets
#
$(PRODUCTS):	Makedep $$@.NEW

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

depend Makedep: $(DERIVED)
	rm -f Makedep
	$(CC) -M $(CDEFS) $(CFILES) $(SFILES) $(DERIVED) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

ALWAYS:

-include Makedep
