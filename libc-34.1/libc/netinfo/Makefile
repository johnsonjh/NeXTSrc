#
# Generic Makefile for libraries
# (c) NeXT, Inc.  1989
#
LIBNAME= netinfolib
BUILDS= STD PROF

CDEFS=	-W -Wall
CFLAGS=	$(CDEFS) -I../../../usr.include -I..
RPCGEN= rpcgen
INCDIR=../../usr.include
IFLAGS= -r -m 444
LIBS=

HFILES=	
GENSRCS = ni_prot_xdr.c ni_prot_clnt.c nb_prot_xdr.c _lu_types_xdr.c \
	  lookupUser.c 
GENHDRS = _lu_types.h lookup.h

SYMLINKS= _lu_types.x lookup.defs
YFILES=
CFILES=	$(GENSRCS) ni_glue.c ni_util.c ni_error.c ni_useful.c	\
	ni_pwdomain.c ni_data.c old_printerdb.c lu_utils.c	\
	lookupUser.c _lu_types_xdr.c getent_pw.c getent_gr.c	\
	getent_h.c getent_s.c getent_r.c getent_n.c getent_p.c	\
	getent_mnt.c getent_prdb.c getent_alias.c 		\
	getent_netgr.c getent_bootp.c getent_bprms.c		\
	putpwpasswd.c
SFILES=

DERIVED= $(GENSRCS) $(GENHDRS)
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

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES) $(GENHDRS)
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

ni_prot_xdr.c: $(INCDIR)/netinfo/ni_prot.x
	$(RPCGEN) -c $? -o $@

#
# For this module, we need to force the otherwise uninitialized statics to be 
# initialized with a sed script in order to get them on the single data
# page of libsys. 
#
ni_prot_clnt.c: $(INCDIR)/netinfo/ni_prot.x
	$(RPCGEN) -l $? | \
	sed 's,\(static[ a-zA-Z0-9_]*\);,\1 = { 0 };,' > $@

nb_prot_xdr.c: $(INCDIR)/netinfo/nibind_prot.x
	$(RPCGEN) -c $? -o $@

_lu_types.x: $(INCDIR)/netinfo/_lu_types.x
	rm -f $@
	ln -s $?

_lu_types_xdr.c: _lu_types.x
	$(RPCGEN) -c $? -o $@

_lu_types.h: _lu_types.x
	$(RPCGEN) -h $? -o $@

lookupUser.c lookup.h: lookup.defs
	mig $?

lookup.defs: $(INCDIR)/netinfo/lookup.defs
	rm -f $@
	ln -s $?
#
# Standard targets
#
depend:
	md -d -u Makedep *.d

clean:
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

ALWAYS: $(GENSRCS) $(GENHDRS)

-include $(DIR)/Makedep
