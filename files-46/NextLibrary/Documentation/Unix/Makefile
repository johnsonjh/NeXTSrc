#
# Top level makefile for the /NextLibrary/Documentation/Unix directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /NextLibrary/Documentation/Unix

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

# SUBDIRS = ManPages

all:
#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

install: ${DSTDIR}
#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
