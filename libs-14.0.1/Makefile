SUBDIRS = usr.lib
BOMDIR = usr/local/BOMs
BOMFILE = libs.user_bom

all clean depend tags:
	 @for i in ${SUBDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@); \
	 done

install:
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	 for i in ${SUBDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@); \
	 done
	 mkdirs $(DSTROOT)/$(BOMDIR)
	 install -c -m 444 $(BOMFILE) $(DSTROOT)/$(BOMDIR)

installsrc:	SRCROOT
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)

SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi
