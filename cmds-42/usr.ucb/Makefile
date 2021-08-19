DIR=	usr.ucb

SUBDIR=	Mail apply biff checknr clear colcrt colrm compress error	\
	expand finger fold from ftp groups head indent last lastcomm	\
	leave lock logger man mkstr more msgs netstat quota rcp 	\
	rdate rdist rlogin rsh rup ruptime rusers rwho script sendmail 	\
	soelim sysline tail talk tcopy telnet tftp tn3270 traceroute 	\
	tset ul unexpand unifdef users vgrind w wc what whereis which 	\
	whoami whois xstr yes


#	vacation symorder gcore systat vmstat
#SUBDIR=	Mail apply biff checknr clear colcrt colrm compress ctags \
#	error ex expand finger fold from ftp grep groups head \
#	indent last lastcomm leave lock logger man mkstr more msgs \
#	printenv quota rcp rdist rlogin rsh rup ruptime rusers rwho \
#	sccs script sendmail soelim sysline tail talk \
#	tcopy telnet tftp tn3270 tset ul unexpand unifdef users vgrind \
#	wc what whereis which whoami whois window xstr yes
#	vacation symorder

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
