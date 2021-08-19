DIR=	CMDS
BOMDIR=/usr/local/BOMS
USERBOM=sgcmds.user_bom

SUBDIR=	bin etc usr.ucb usr.bin usr.etc

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
