DIR=	usr.bin

SUBDIR=	ar11 at bc bm cal calendar cb checkeq col comm cpio crypt dc 	\
	deroff diction diff3 eqn hostinfo iostat less look mesg neqn 	\
	nohup nroff ptx ratfor rcs rev shar sleep spell spline split 	\
	struct sum tabs tbl tc tip tk troff tty units uucp vm_stat 	\
	wh xargs xsend ypcat ypmatch yppasswd ypwhich

# sup tip graph plot prof rcs learn chkey keylogin blit

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
