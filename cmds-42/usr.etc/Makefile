DIR=	usr.etc

SUBDIR=	autonfsmount checkswap exportfs mach_init mach_swapon		\
	mkfile nfsstat nmserver ntp rpc.rexd rpc.bootparamd rpc.lockd	\
	rpc.mountd rpc.pcnfsd rpc.rquotad rpc.rstatd rpc.statd		\
	rpc.rusersd rpc.rwalld rpc.sprayd rpc.yppasswdd rpcinfo		\
	rwall showmount spray yp ypserv

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

