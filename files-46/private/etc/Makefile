#
# Makefile for the /private/etc directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/etc

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = staff

#	Files that we don't want people to edit
ROFILES = fstab.client fstab.od fstab.sd330 fstab.sd660 fstab.sd660.1	\
	fstab.fd rc rc.boot rc.swap rc.bldsd100 rc.ldsd100 		\
	rc.boot.ldsd100 rc.uucp ttys.installer
ROMODE = 444

#	Files that may be overwritten during floppy boot process, so
#	we keep a .standard copy
STDFILES = rc rc.boot

#	Files that are ok to edit
RWFILES	= crontab exports.example group hostconfig hosts networks \
	passwd protocols printcap rc.local rpc services shells termcap ttys \
	kern_loader.conf bootstrap.conf
RWMODE = 644

#	Files that are ok to edit that are initially empty
EMPTYFILES = dumpdates find.codes hosts.equiv hosts.lpd motd \
	netgroup rmtab syslog.pid utmp xtab
EMPTYMODE = 644

#	Initially empty files that are ok for the group to edit.
GROUPEMPTYFILES = mtab
GROUPEMPTYMODE = 664
GROUPEMPTYGROUP = wheel

#	Subdirectories with their own Makefiles
SUBDIRS = yp

#	It doesn't mean anything to make in this directory
all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	echo -n "    Read only files:"
	for i in `echo ${ROFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${ROMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Standard files:"
	for i in `echo ${STDFILES}` ; \
	  do \
		echo -n " $$i.standard" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${ROMODE} $$i ${DSTDIR}/$$i.standard ; \
	  done
	echo "."
	echo -n "    Read/Write files:"
	for i in `echo ${RWFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${RWMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Empty files:"
	for i in `echo ${EMPTYFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${GROUP} ${DSTDIR}/$$i ; \
		chmod ${EMPTYMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Empty group-writeable files:"
	for i in `echo ${GROUPEMPTYFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${GROUPEMPTYGROUP} ${DSTDIR}/$$i ; \
		chmod ${GROUPEMPTYMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi

