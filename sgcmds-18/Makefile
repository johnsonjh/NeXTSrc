DIR=	SGCMDS
BOMDIR=/usr/local/BOMS
USERBOM=sgcmds.user_bom

#SUBDIR=	bin lib usr.bin usr.ucb usr.local usr.lib
SUBDIR=	bin usr.bin usr.ucb usr.local etc

all clean depend tags: ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '#################' make $@ for $(DIR) done '###################'

install: DSTROOT ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@ || exit 1); \
	done
#
#       install -c -m 444 ${USERBOM} ${DSTROOT}${BOMDIR}
#
	@echo '#################' make $@ for $(DIR) done '###################'

installsrc:	SRCROOT
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)

DSTROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

ALWAYS:
