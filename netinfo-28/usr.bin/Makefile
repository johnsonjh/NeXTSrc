SUBDIRS = niload nidump niutil

all clean depend:
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

installsrc:
	cp Makefile $(SRCROOT)
	@CWD=`pwd`; cd ${SRCROOT}; SRCROOT=`pwd`; cd $$CWD; \
	 for i in ${SUBDIRS}; \
	 do \
		echo ================= make $@ for $$i =================; \
		mkdirs $$SRCROOT/$$i; \
		(cd $$i; ${MAKE} SRCROOT=$$SRCROOT/$$i $@); \
	 done

SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

