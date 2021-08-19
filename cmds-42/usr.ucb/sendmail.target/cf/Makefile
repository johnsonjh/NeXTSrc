#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= sendmail.main.cf sendmail.subsidiary.cf
ETCDIR= $(DSTROOT)/private/etc/sendmail
DSTDIRS= $(ETCDIR)

IFLAGS= -c -m 444

M4FILES=main.mc subsidiary.mc base.m4 berkm.m4 cchosts.m4 compat.m4 \
	ddnm.m4 ether.m4 etherm.m4 fudge.m4 ibmm.m4 localm.m4 \
	mmdfm.m4 nextbase.m4 suucpm.m4 uucpm.m4 uucpproto.mc \
	version.m4 zerobase.m4

GARBAGE= main.cf subsidiary.cf

.SUFFIXES: .mc .cf

all:	$(PRODUCTS)

# Production rules
.mc.cf:
	m4 $*.mc > $*.cf

install: DSTROOT $(DSTDIRS) all symlinks
	install ${IFLAGS} sendmail.main.cf.NEW ${ETCDIR}/sendmail.main.cf
	install ${IFLAGS} sendmail.subsidiary.cf.NEW ${ETCDIR}/sendmail.subsidiary.cf
	install -c -m 644 sendmail.subsidiary.cf.NEW ${ETCDIR}/sendmail.cf

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

#
# Building main.cf
#
MAIN_OBJS= nextbase.m4 etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 \
	zerobase.m4 suucpm.m4 uucpm.m4 compat.m4 main.mc

sendmail.main.cf.NEW:	${MAIN_OBJS} main.cf
	cp main.cf sendmail.main.cf.NEW

#
# Building subsidiary.cf
#
SUBSIDIARY_OBJS= nextbase.m4 etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 \
	zerobase.m4 suucpm.m4 uucpm.m4 compat.m4 subsidiary.mc

sendmail.subsidiary.cf.NEW:	${SUBSIDIARY_OBJS} subsidiary.cf
	cp subsidiary.cf sendmail.subsidiary.cf.NEW

#
# Standard targets
#
$(PRODUCTS):	$$@.NEW

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

depend tags:

ALWAYS:

-include Makedep
