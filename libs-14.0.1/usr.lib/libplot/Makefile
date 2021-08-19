#	Makefile	4.10	85/09/21
#
DSTDIR=
LIBDIR=$(DSTROOT)$(DSTDIR)/usr/lib

ALL=	libf77plot libplot lib300 lib300s lib4013 lib4014 lib450 libvt0 \
	libplotaed libplotbg libplotdumb libplotgigi libplot2648 \
	libplot7221 libplotimagen
SUBDIRS=tf77 plot t4013 t4014 t300 t300s t450 vt0 \
	aed bitgraph dumb gigi hp2648 hp7221 imagen

all:	${ALL}

libf77plot: FRC
	cd tf77; $(MAKE) ${MFLAGS}

libplot: FRC
	cd plot; $(MAKE) ${MFLAGS}

lib4013: FRC
	cd t4013; $(MAKE) ${MFLAGS}

lib4014: FRC
	cd t4014; $(MAKE) ${MFLAGS}

lib300: FRC
	cd t300; $(MAKE) ${MFLAGS}

lib300s: FRC
	cd t300s; $(MAKE) ${MFLAGS}

lib450: FRC
	cd t450; $(MAKE) ${MFLAGS}

libvt0: FRC
	cd vt0; $(MAKE) ${MFLAGS}

libplotaed: FRC
	cd aed; $(MAKE) ${MFLAGS}

libplotbg: FRC
	cd bitgraph; $(MAKE) ${MFLAGS}

libplotdumb: FRC
	cd dumb; $(MAKE) ${MFLAGS}

libplotgigi: FRC
	cd gigi; $(MAKE) ${MFLAGS}

libplot2648: FRC
	cd hp2648; $(MAKE) ${MFLAGS}

libplot7221: FRC
	cd hp7221; $(MAKE) ${MFLAGS}

libplotimagen: FRC
	cd imagen; $(MAKE) ${MFLAGS}

FRC:

install: ckdstroot all
	for i in ${ALL}; do \
		install -m 644 $$i $(LIBDIR)/$$i.a; \
		ranlib $(LIBDIR)/$$i.a; \
	done

ckdstroot:
	@if [ -n "${DSTROOT}" ]; \
	then \
		exit 0; \
	else \
		echo Must define DSTROOT; \
		exit 1; \
	fi

clean:
	rm -f ${ALL} errs a.out core
	for i in ${SUBDIRS}; \
	do \
		echo ==================== $$i ====================; \
		(cd $$i; $(MAKE) ${MFLAGS} clean); \
	done

depend:
	@echo No depend target, make depend ignored
