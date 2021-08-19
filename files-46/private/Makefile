#
# Top level makefile for the /private directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

#	We only need to make the template directory.  Private is made
#	with the builddisk script, and other clients are made with
#	the newclient script.
SUBDIRS = adm dev etc spool tftpboot vm

#	Empty subdirectories
EMPTYDIRS = preserve Net

#	Temp directory
TMPDIR = tmp
TMPMODE = 1777

all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	echo "    Making tmp"
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${TMPMODE} ${DSTDIR}/${TMPDIR}

	echo -n "    Empty directories:"
	for i in ${EMPTYDIRS} ; \
	  do \
		echo -n " $$i" ; \
		mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
