#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCT= ftpd
BINDIR=	$(DSTROOT)/usr/etc
CONFDIR= $(DSTROOT)/private/etc
DSTDIRS= $(BINDIR) $(CONFDIR)

#
# libsys won't allow us to override strpbrk. 
# We redefine it here.
#
STRPBRKDEF= -Dstrpbrk=ftpd_strpbrk

CDEFS=	-DNeXT_MOD -DSETPROCTITLE $(STRPBRKDEF)
CFLAGS=	-O -g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS= -lsys_s

HFILES=
YFILES=	ftpcmd.y
CFILES= ${PRODUCT}.c glob.c getusershell.c logwtmp.c popen.c strcasecmp.c  \
	strpbrk.c strtok.c
SFILES=

DERIVED= version.c
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o) version.o

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all symlinks
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)
	install -c -m 644 ftpusers $(CONFDIR)

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

#
# Building PRODUCT
#

$(PRODUCT).NEW:	$(OBJS) 
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

version.c: newvers.sh $(SRCS)
	./newvers.sh

#
# Standard targets
#
$(PRODUCT):	Makedep $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW TAGS tags vers.c Makedep $(PRODUCT) $(DERIVED) \
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
