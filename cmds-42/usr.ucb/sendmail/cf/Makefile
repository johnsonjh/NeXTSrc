#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
PRODUCTS= sendmail.mailhost.cf sendmail.subsidiary.cf \
	sendmail.sharedsubsidiary.cf
ETCDIR= $(DSTROOT)/private/etc/sendmail
DSTDIRS= $(ETCDIR)

IFLAGS= -c -m 444

M4FILES=mailhost.mc subsidiary.mc base.m4 berkm.m4 cchosts.m4 compat.m4 \
	ddnm.m4 ether.m4 etherm.m4 fudge.m4 ibmm.m4 localm.m4 \
	mmdfm.m4 nextbase.m4 suucpm.m4 uucpm.m4 uucpproto.mc \
	version.m4 zerobase.m4 etherlm.m4 sharedsubsidiary.mc

GARBAGE= mailhost.cf subsidiary.cf sharedsubsidiary.cf

.SUFFIXES: .mc .cf

all:	$(PRODUCTS)

# Production rules
.mc.cf:
	m4 $*.mc > $*.cf

install: DSTROOT $(DSTDIRS) all symlinks
	install ${IFLAGS} sendmail.mailhost.cf.NEW ${ETCDIR}/sendmail.mailhost.cf
	install ${IFLAGS} sendmail.subsidiary.cf.NEW ${ETCDIR}/sendmail.subsidiary.cf
	install ${IFLAGS} sendmail.sharedsubsidiary.cf.NEW ${ETCDIR}/sendmail.sharedsubsidiary.cf
	ln -s sendmail.subsidiary.cf ${ETCDIR}/sendmail.cf

symlinks:
	@if [ ! -h ${DSTROOT}/etc ] ; \
	then \
		echo "ln -s private/etc ${DSTROOT}/etc" ; \
		ln -s private/etc ${DSTROOT}/etc ; \
	fi

#
# Building mailhost.cf
#
MAILHOST_OBJS= nextbase.m4 etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 \
	zerobase.m4 suucpm.m4 uucpm.m4 compat.m4 mailhost.mc

sendmail.mailhost.cf.NEW:	${MAILHOST_OBJS} mailhost.cf
	cp mailhost.cf sendmail.mailhost.cf.NEW

#
# Building subsidiary.cf
#
SUBSIDIARY_OBJS= nextbase.m4 etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 \
	zerobase.m4 suucpm.m4 uucpm.m4 compat.m4 subsidiary.mc

sendmail.subsidiary.cf.NEW:	${SUBSIDIARY_OBJS} subsidiary.cf
	cp subsidiary.cf sendmail.subsidiary.cf.NEW

#
# Building sharedsubsidiary.cf
#
SHAREDSUBSIDIARY_OBJS= nextbase.m4 etherm.m4 base.m4 localm.m4 ddnm.m4 version.m4 \
	zerobase.m4 suucpm.m4 uucpm.m4 compat.m4 etherlm.m4 sharedsubsidiary.mc

sendmail.sharedsubsidiary.cf.NEW:	${SHAREDSUBSIDIARY_OBJS} sharedsubsidiary.cf
	cp sharedsubsidiary.cf sendmail.sharedsubsidiary.cf.NEW

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
