#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= rpclib
BUILDS= STD PROF

CDEFS=	
CFLAGS=	$(CDEFS) -I../.. -I../../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	
YFILES=
CFILES=	auth_none.c auth_unix.c authdes_prot.c authunix_prot.c	\
	bindresvport.c clnt_generic.c clnt_perror.c clnt_raw.c	\
	clnt_simple.c clnt_tcp.c clnt_udp.c rpc_dtablesz.c	\
	get_myaddress.c getrpcent.c getrpcport.c key_call.c	\
	key_prot.c mcast_args.c netname.c openchild.c		\
	pmap_clnt.c pmap_getmaps.c pmap_getport.c pmap_prot.c	\
	pmap_prot2.c pmap_rmt.c rpc_prot.c rpc_comdata.c	\
	rpc_callmsg.c rtime.c svc.c svc_auth.c svc_auth_unix.c	\
	svc_raw.c svc_run.c svc_simple.c svc_tcp.c svc_udp.c	\
	svcauth_des.c xdr.c xdr_array.c xdr_float.c xdr_mem.c	\
	xdr_rec.c xdr_reference.c xdr_stdio.c
SFILES=

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
	$(CC) -MD $(CFLAGS) $(COPTS) -c -o $@ $<

all:	$(BUILDS)

SHLIB PROF STD:	$$@_OBJS ALWAYS
	cd $@_OBJS; $(MAKE) -f ../Makefile TYPE="$@" $(LIBNAME)$($@_NAME)

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
