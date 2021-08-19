DIR=	usr.bin

SUBDIR=	basename egrep fgrep find install join lex lorder m4 md mig	\
	refer rpcgen sort touch tr tsort uniq vers_string yacc 

all clean depend tags: ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '#################' make $@ for $(DIR) done '###################'

install: ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@ || exit 1); \
	done
	@echo '#################' make $@ for $(DIR) done '###################'

ALWAYS:
