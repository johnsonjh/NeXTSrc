#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
#	Makefile	7.8	83/07/09
VERSION=3.7
#
# Ex skeletal makefile for VAX VM/Unix 4.2BSD
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Ex is very large - this version will not fit on PDP-11's without overlay
# software.  Things that can be turned off to save
# space include LISPCODE (-l flag, showmatch and lisp options), UCVISUAL
# (visual \ nonsense on upper case only terminals), CHDIR (the undocumented
# chdir command.)  CRYPT includes the code to edit encrypted files (the -x
# option, like ed.)  VMUNIX makes ex considerably larger, raising many limits
# and improving speed and simplicity of maintenance.  It is suitable only
# for a VAX or other large machine, and then probably only in a paged system.
#
# Don't define VFORK unless your system has the VFORK system call,
# which is like fork but the two processes have only one data space until the
# child execs. This speeds up ex by saving the memory copy.
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
PRODUCTS= ex expreserve exrecover
BINDIR=	$(DSTROOT)/usr/ucb
LIBDIR=$(DSTROOT)/usr/lib
PRSDIR=$(DSTROOT)/private/preserve
DSTDIRS= $(BINDIR) ${LIBDIR}

CDEFS=	-DNeXT_MOD -D__STRICT_BSD__ -DCRYPT -DLISPCODE -DCHDIR -DUCVISUAL \
	-DVFORK -DVMUNIX -DSTDIO -DMAD -DNEW_MAD -DFASTTAG -DTABS=8
#CFLAGS=	-O -fwritable-strings $(CDEFS)
CFLAGS=	-g -fwritable-strings $(CDEFS)
IFLAGS= -s
LIBS=	-lsys_s -ltermcap

HFILES= ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h \
	ex_vis.h uparm.h
YFILES=
CFILES= ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c ex_data.c \
	ex_extern.c ex_fstk.c ex_get.c ex_io.c ex_put.c ex_re.c ex_set.c \
	ex_subr.c ex_tagio.c ex_temp.c ex_tty.c ex_unix.c ex_v.c ex_vadj.c \
	ex_vget.c ex_vmain.c ex_voper.c ex_vops.c ex_vops2.c ex_vops3.c \
	ex_vput.c ex_vwind.c expreserve.c exrecover.c printf.c
SFILES=

DERIVED=
GARBAGE=

SRCS= $(HFILES) $(YFILES) $(CFILES) $(SFILES)
OBJS= $(CFILES:.c=.o) $(YFILES:.y=.o) $(SFILES:.s=.o)

all:	$(PRODUCTS)

install: DSTROOT $(DSTDIRS) all symlinks
	-rm -f ${BINDIR}/ex
	-rm -f ${BINDIR}/vi
	-rm -f ${BINDIR}/view
	-rm -f ${BINDIR}/edit
	install $(IFLAGS) -m 1755 ex.NEW ${BINDIR}/ex
	ln ${BINDIR}/ex ${BINDIR}/edit
	ln ${BINDIR}/ex ${BINDIR}/vi
	ln ${BINDIR}/ex ${BINDIR}/view
	install $(IFLAGS) -m 4755 exrecover.NEW ${LIBDIR}/ex${VERSION}recover
	install $(IFLAGS) -m 4755 expreserve.NEW ${LIBDIR}/ex${VERSION}preserve

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi
	@if [ ! -h ${DSTROOT}/tmp ] ; \
	then \
		echo "ln -s private/tmp ${DSTROOT}/tmp" ; \
		ln -s private/tmp ${DSTROOT}/tmp ; \
	fi
	@if [ ! -h ${DSTROOT}/usr/preserve ] ; \
	then \
		echo "ln -s ../private/preserve ${DSTROOT}/usr/preserve" ; \
		ln -s ../private/preserve ${DSTROOT}/usr/preserve ; \
	fi

#
# Building ex
#
EX_OBJS	=ex.o ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o ex_data.o \
	ex_extern.o ex_get.o ex_io.o ex_put.o ex_re.o ex_set.o ex_subr.o \
	ex_temp.o ex_tty.o ex_unix.o ex_v.o ex_vadj.o ex_vget.o ex_vmain.o \
	ex_voper.o ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	printf.o ex_fstk.o ex_tagio.o

ex.NEW:	$(EX_OBJS) ex_vers.o
	$(CC) $(CFLAGS) -o $@ $(EX_OBJS) ex_vers.o $(LIBS)

ex_vers.c: $(DOVERS) $(EX_OBJS)
	@rm -f $@
	vers_string -c ex > $@

#
# Building exrecover
#
EXRECOVER_OBJS= exrecover.o ex_extern.o

exrecover.NEW:	$(EXRECOVER_OBJS) exrecover_vers.o
	$(CC) $(CFLAGS) -o $@ $(EXRECOVER_OBJS) exrecover_vers.o $(LIBS)

exrecover_vers.c: $(DOVERS) $(EXRECOVER_OBJS)
	@rm -f $@
	vers_string -c exrecover > $@

#
# Building expreserve
#
EXPRESERVE_OBJS= expreserve.o

expreserve.NEW:	$(EXPRESERVE_OBJS) expreserve_vers.o
	$(CC) $(CFLAGS) -o $@ $(EXPRESERVE_OBJS) expreserve_vers.o $(LIBS)

expreserve_vers.c: $(DOVERS) $(EXPRESERVE_OBJS)
	@rm -f $@
	vers_string -c expreserve > $@

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
