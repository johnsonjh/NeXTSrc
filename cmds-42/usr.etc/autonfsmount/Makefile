#
# Generic top-level makefile for simple commands
# @(#)Makefile	1.6 88/08/08 4.0NFSSRC SMI
# (c) NeXT, Inc.  1987
#
PRODUCT= autonfsmount
BINDIR=	$(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

CDEFS=	-DNeXT -bsd

CFLAGS=	-g $(CDEFS) -O
#CFLAGS=	-pg -g $(CDEFS) -O
LDFLAGS= -sectorder __TEXT __text callf.order
IFLAGS= -s
LIBS= -lsys_s -lrpcsvc

XFILES= nfs_prot.x
GENCFILES= nfs_prot.c
GENHFILES= nfs_prot.h
HFILES= automount.h $(GENHFILES)
YFILES=
CFILES= $(GENCFILES) nfs_server.c \
	auto_main.c auto_look.c auto_proc.c auto_node.c \
	auto_mount.c auto_all.c strdup.c auto_fstab.c fslist.c \
	many_cast.c auto_site.c nfs_trace.c
SFILES=

DERIVED= $(GENCFILES) $(GENHFILES)
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCT)

install: DSTROOT $(DSTDIRS) all
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

#
# Building PRODUCT
#

$(PRODUCT).NEW:	$(OBJS) vers.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

nfs_prot.c: nfs_prot.x
	rpcgen -c nfs_prot.x -o $@

nfs_prot.h: nfs_prot.x
	rpcgen -h nfs_prot.x -o $@

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
