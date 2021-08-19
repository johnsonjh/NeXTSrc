#
# Makefile for the /usr/template directory.
#

# Comment out this line if you want all of the make commands output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /usr/template

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	The default account
DEFAULTACCOUNT = me
DEFAULTUID = 20
DEFAULTGROUP = 20

#	Where we build accounts
ACCOUNTDIR = ${DSTROOT}

#	Subdirectories without their own makefiles
SLAVEDIRS = user

all:
#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SLAVEDIRS} ; \
	  do \
		echo "    Removing" $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ;\
		rm -f $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ; \
	  done
#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

install: ${DSTDIR}
	for i in ${SLAVEDIRS} ; \
	  do \
		echo " Removing old" ${DSTDIR}/$$i ; \
		rm -rf ${DSTDIR}/$$i ; \
		echo " Copying" $$i to ${DSTDIR}/$$i ; \
		cp -r $$i ${DSTDIR} ; \
		echo " Setting" ${DSTDIR}/$$i to ug=rwX,o=rX ; \
		chmod -R ug=rwX,o=rX ${DSTDIR}/$$i ; \
	  done
	echo " Setting ${DSTDIR}/user/Apps/README.wn to be read-only."
	chmod 444 ${DSTDIR}/user/Apps/README.wn
	echo "    Make the default account..."
	echo "      Removing contents of ${ACCOUNTDIR}/${DEFAULTACCOUNT}"
	rm -rf ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	echo "      Copying" template to ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	cp -r user ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	echo "      Changing" ownership of ${ACCOUNTDIR}/${DEFAULTACCOUNT} to ${DEFAULTUID}.${DEFAULTGROUP}
	chown -R ${DEFAULTUID}.${DEFAULTGROUP} ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	echo "      Setting" ${ACCOUNTDIR}/${DEFAULTACCOUNT} to u=rwX,go=rX
	chmod -R u=rwX,go=rX ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	chmod 777 ${ACCOUNTDIR}/${DEFAULTACCOUNT}
	echo " Setting ${ACCOUNTDIR}/${DEFAULTACCOUNT}/Apps/README.wn to be read-only."
	chmod 444 ${ACCOUNTDIR}/${DEFAULTACCOUNT}/Apps/README.wn

#	for i in ${SUBDIRS} ; \
#	  do \
#		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
#		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
#	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	mkdirs -o ${DSTOWNER} -g ${DSTGROUP} -m ${DSTMODE} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
