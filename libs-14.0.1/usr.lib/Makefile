# This Makefile is derived from the standard parameterization
# file for describing one or more products in a single directory 
# and associated sub directories.
# Copyright 1988, NeXT, Inc.
#
# The following are conspicuously absent from this files:
#	libobjc # we should build this as soon as gcc can
#	libF77 libI77 libU77   # we're not using f77
#	libpc	# we're not using pc
#	libplot # surely we'll have something better!?
#	learn	# does anybody really use learn?
#	libom	# I see no reason to support the old math library
#	lpr	# this now lives in ucb (it has to for complete system builds
#		# since it needs libs and they aren't built yet!)
#	liboldnm# ancient math library
#	lib2648 # for HP2648 graphics terminal

SUBDIR = lib.b libcurses libdbm libg libln libmp libtermlib liby \
	librpcsvc makewhatis getNAME makekey

# Special rule for a master tags file at this level
# Subdirectories whose routines are included in the making of the
# master tags file (/usr/lib/tags)

TAGSFILE=tags
TAGSDIR=libcurses libdbm libln libmp libtermlib

tags:
	-@for i in ${TAGSDIR}; do \
		(cd $$i; $(MAKE) ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done

all clean depend:
	 @for i in ${SUBDIR}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} $@); \
	 done

install:
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	 for i in ${SUBDIR}; \
	 do \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@); \
	 done
