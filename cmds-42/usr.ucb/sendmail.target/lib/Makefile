#
# Generic top-level makefile
# (c) NeXT, Inc.  1987
#
LIBDIR=	$(DSTROOT)/usr/lib
CONFDIR= $(DSTROOT)/private/etc/sendmail
DSTDIRS= $(LIBDIR) $(CONFDIR)

all depend tags clean:

install: DSTROOT $(DSTDIRS) all symlinks
	install -c -m 644 -o root sendmail.hf $(LIBDIR)/sendmail.hf
	install -c -m 664 -o root -g staff aliases $(CONFDIR)/aliases

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

DSTROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS}:
	mkdirs $@

ALWAYS:
