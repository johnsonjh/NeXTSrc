PRODUCT=lookupd
BINDIR= $(DSTROOT)/usr/etc
DSTDIRS= $(BINDIR)

-include ../../lib/Makefile.optimize

WARNINGS= -W -Wall
#MALLOC_DEBUG= -DMALLOC_DEBUG

CFLAGS= $(OPTIMIZE) -DNeXT_MOD $(WARNINGS) $(MALLOC_DEBUG)
LDFLAGS= $(OPTIMIZE) $(SCATTER_LOAD)

LIBS= $(LIBC)
LD= ld
CRT0= -lcrt0.c
IFLAGS= -s
REAL_SRCS= lookup_main.c lookup_proc.c \
	copy_mem.c debug.c \
	lookup_pw.c lookup_gr.c lookup_h.c lookup_n.c lookup_s.c \
	lookup_p.c lookup_r.c lookup_mnt.c lookup_prdb.c \
	lookup_alias.c lookup_bootparams.c lookup_bootp.c lookup_netgroup.c \
	ni_getent.c getent.c nettoa.c checksum.c _yp_running.c 

REAL_HDRS=copy_mem.h mm.h clib.h clib_internal.h libc_redefines.h checksum.h

REAL_OBJS=$(REAL_SRCS:.c=.o)

DERIVED_HDRS= lookup.h _lu_types.h
DERIVED_SRCS= _lu_types_xdr.c lookupServer.c lookupUser.c
DERIVED= $(DERIVED_HDRS) $(DERIVED_SRCS)

GARBAGE= _lu_types.x lookup.defs 

DERIVED_OBJS=$(DERIVED_SRCS:.c=.o)

SRCS=$(REAL_SRCS) $(DERIVED_SRCS) 
OBJS=$(DERIVED_OBJS) $(REAL_OBJS)

all: $(PRODUCT)

$(PRODUCT).NEW: $(OBJS) vers.o
	$(CC) -o $@ $(LDFLAGS) $(OBJS) vers.o $(LIBS)

check:  $(OBJS) vers.o
	unref $(OBJS) vers.o $(LIBS)

vers.c: $(DOVERS) $(OBJS)
	@rm -f $@
	vers_string -c $(PRODUCT) > $@

_lu_types.x: /usr/include/netinfo/_lu_types.x
	ln -s $?

lookup.defs: /usr/include/netinfo/lookup.defs
	ln -s $?

lookup.h lookupUser.c lookupServer.c: lookup.defs
	mig $?

_lu_types_xdr.c: _lu_types.x
	rpcgen -c $? -o $@
	
_lu_types.h: _lu_types.x
	rpcgen -h $? -o $@

$(PRODUCT): Makedep $$@.NEW

installsrc: $(REAL_SRCS) $(REAL_HDRS) Makefile gmon.order
	cp $? $(SRCROOT)


install: $(PRODUCT) DSTROOT $(DSTDIRS) 
	install $(IFLAGS) $(PRODUCT).NEW $(BINDIR)/$(PRODUCT)

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
	$(CC) -M $(CFLAGS) $(SRCS) | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep

tags:	ALWAYS
	etags $(SRCS)
	ctags $(SRCS)

-include Makedep

ALWAYS:



