DIR=	etc
CONFDIR= ${DSTROOT}/private/etc
DSTDIRS= ${CONFDIR}

SUBDIR=	ac accton arp autodiskmount biod bootpd catman clri comsat	\
	cron dcheck disk dmesg dump dump.old dumpfs edquota fbshow	\
	fdform fingerd fsck fsirand ftpd getdisk gettable getty halt	\
	htable icheck ifconfig inetd init kgmon mkfs mkhosts 		\
	mklost+found mknod mkpasswd mkproto mount named ncheck		\
	newclient newfs nfsd nppower nslookup nu ping portmap quot	\
	quotacheck quotaon  reboot renice repquota restore 		\
	restore.old rexecd rlogind rmt route routed rshd rwhod sa	\
	scsimodes sdform shutdown syslogd talkd telnetd tftpd timed 	\
	tunefs tzone umount update vipw vmmprint vmoprint ypbind 	\
	zprint

# TRIAGE die: XNSrouted swapon mkSingleUser rpc.lockd implog

SYMLINKS= catman chown \
	disk dump rdump edquota \
	fsirand halt ifconfig inetd \
	init mkfs mkhosts mklost+found mknod mkpasswd mkplaces mkproto \
	mount newclient newfs nu ping quot \
	quotacheck quotaon reboot renice repquota restore rrestore rexecd \
	rlogind rmt route rshd rwhod \
	shutdown umount update vipw

all clean depend tags: ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '#################' make $@ for $(DIR) done '###################'

install: DSTROOT ${DSTDIRS} ALWAYS
	@echo '#################' make $@ for $(DIR) '###################'
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@ || exit 1); \
	done
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	for i in ${SYMLINKS}; \
	do \
		rm -f ${DSTROOT}/private/etc/$$i; \
		echo "ln -s ../../usr/etc/$$i ${DSTROOT}/private/etc/$$i" ; \
		ln -s ../../usr/etc/$$i ${DSTROOT}/private/etc/$$i ; \
	done
	@echo '#################' make $@ for $(DIR) done '###################'

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

ALWAYS:
