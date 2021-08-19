#
# Generic Makefile for include file source directories
# (c) NeXT, Inc.  1987
#
# The only work here is to drop the include files specified by this
# directory into the specified DSTROOT.  Some care is given to avoid
# rewriting include files that have not changed.
#
# Note that this makefile does not deal with kernel include files,
# the kernel makefile is responsable for installing these.  Note,
# also, that we don't mess around with symbolic links to kernel
# includes, we force these to be explicitly installed.
#

# These are always overridden
TOFILE=none
FROMFILE=none

IFLAGS= -c -m 444

PROJECT= include
PRODUCT= include

DSTDIR= 
INCDIR=	${DSTDIR}/usr/include

# User header files include all of the *.h and *.i files in this
# directory and each subdirectory and all of the .h files derived 
# from the .defs files.

REGDEFS=	mach.defs exc.defs mach_host.defs
SERVERDEFS=	servers/bootstrap.defs servers/netname.defs

DEFS=	exc.defs mach_host.defs \
	servers/bootstrap.defs servers/netname.defs

RPCDEFS= netinfo/ni_prot.x netinfo/nibind_prot.x

DERIVED_HFILES= $(DEFS:.defs=.h) $(RPCDEFS:.x=.h) mach_interface.h
USER_CFILES= $(DEFS:.defs=_u.c) mach_u.c
SERVER_CFILES= $(DEFS:.defs=_server.c) mach_server.c

GARBAGE= $(DERIVED_HFILES) $(USER_CFILES) $(SERVER_CFILES) TAGS tags
SUBDIRS= netinfo
HDRDIRS = arpa netinfo next protocols rpc rpcsvc servers

all: $(DERIVED_HFILES)
	@for i in ${SUBDIRS}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done

depend:

# Of the following three rules, whichever gets activated first
# will satisfy the contraints of the other two.  Unfortunately,
# a change to any def file will trigger the others also.

DOMIG = basedefsname=`basename $$defsname`;\
	mig -q $$defsname.defs -I. \
		-header $$defsname.h \
		-user `echo $$defsname`_u.c \
		-server `echo $$defsname`_server.c;

KERNHDR = rm -f $$defsname.h; \
	echo "#import <kern/$$defsname.h>" > $$defsname.h

mach_interface.h mach_u.c mach_server.c: mach.defs $(INCDIR)/kern/mach.defs
	mig -q mach.defs -I. \
		-header mach_interface.h \
		-user mach_u.c \
		-server mach_server.c

exc.h exc_u.c exc_server.c: exc.defs $(INCDIR)/kern/exc.defs
	defsname=exc; $(DOMIG) $(KERNHDR)

mach_host.h mach_host_u.c mach_host_server.c: mach_host.defs $(INCDIR)/kern/mach_host.defs
	defsname=mach_host; $(DOMIG) $(KERNHDR)

servers/bootstrap.h servers/bootstrap_u.c servers/bootstrap_server.c: servers/bootstrap.defs
	defsname=servers/bootstrap; $(DOMIG)

servers/netname.h servers/netname_u.c servers/netname_server.c: servers/netname.defs
	defsname=servers/netname; $(DOMIG)

interface: TOFILE FROMFILE $(TOFILE)

netinfo/ni_prot.h: netinfo/ni_prot.x
	cd netinfo; $(MAKE) ni_prot.h

netinfo/nibind_prot.h: netinfo/nibind_prot.x
	cd netinfo; $(MAKE) nibind_prot.h

$(TOFILE): $(FROMFILE)
	rm -f ${TOFILE}
	cp $(FROMFILE) $(TOFILE)

install: all DSTROOT ${DSTROOT}${INCDIR} $(DERIVED_HFILES) 
	install -d ${IFLAGS} *.h ${DSTROOT}${INCDIR};
	install -d ${IFLAGS} std_types.defs mach_types.defs ${DSTROOT}${INCDIR}
	install ${IFLAGS} ${REGDEFS} ${DSTROOT}${INCDIR}
	mkdirs ${DSTROOT}${INCDIR}/servers
	install ${IFLAGS} ${SERVERDEFS} ${DSTROOT}${INCDIR}
	for i in ${HDRDIRS}; do \
		if [ ! -d ${DSTROOT}${INCDIR}/$$i ]; \
		then \
			rm -f ${DSTROOT}${INCDIR}/$$i; \
			mkdirs ${DSTROOT}${INCDIR}/$$i; \
		fi; \
		(cd $$i; \
		 install -d ${IFLAGS} *.[ihx] ${DSTROOT}${INCDIR}/$$i; \
		) || exit 1; \
	done
	install -d ${IFLAGS} netinfo/lookup.defs ${DSTROOT}${INCDIR}
	-rm -f $(DSTROOT)${INCDIR}/machine
	ln -s next $(DSTROOT)${INCDIR}/machine

tags: *.h arpa/*.h protocols/*.h $(DERIVED_HFILES)
	ctags -w *.h arpa/*.h protocols/*.h $(DERIVED_HFILES)

TAGS: *.h arpa/*.h protocols/*.h $(DERIVED_HFILES)
	etags *.h arpa/*.h protocols/*.h $(DERIVED_HFILES)

clean:
	-rm -f ${GARBAGE}

DSTROOT LCLROOT TOFILE FROMFILE:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTROOT}${INCDIR} ${LCLROOT}${INCDIR}:
	mkdirs $@

ALWAYS:



