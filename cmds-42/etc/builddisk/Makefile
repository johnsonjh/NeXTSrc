#
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987
#
PRODUCT= builddisk
BLDFILES= BLD.od BLD.sd200app BLD.sd200dev BLD.sd330 BLD.sd660 BLD.swapdisk BLD.bs BLD.fd_to_sd
BINDIR=	$(DSTROOT)/usr/etc
CONFDIR=${DSTROOT}/private/etc
DSTDIRS= ${BINDIR} ${CONFDIR}

IFLAGS= -xc -m 555

all:	$(PRODUCT).NEW

$(PRODUCT).NEW:	$(PRODUCT).csh $(DOVERS)
	sed -e "s/#PROGRAM.*/#`vers_string $(PRODUCT)`/" \
	    <$(PRODUCT).csh >$(PRODUCT).NEW

clean:	ALWAYS
	-rm -f *.NEW

install: DSTROOT ${DSTDIRS} all symlinks
	install ${IFLAGS} ${PRODUCT}.NEW ${BINDIR}/${PRODUCT}
	install -c ${BLDFILES} ${CONFDIR}

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

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

depend tags ALWAYS:
